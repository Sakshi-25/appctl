#include <librlxpkg/librlxpkg.hh>
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>
#include <curl/curl.h>
#include <math.h>
#include <string.h>

using namespace libapp;
using namespace std;

typedef union _uwb {
    u_int w;
    u_char b[4];
} md5_union_t;


typedef u_int _dgst_arr[4];

u_int _f0(u_int a[]) { return (a[1] & a[2]) | (~a[1] & a[3]); }
u_int _f1(u_int a[]) { return (a[3] & a[1]) | (~a[3] & a[2]); }
u_int _f2(u_int a[]) { return (a[1] ^ a[2] ^ a[3]);           }
u_int _f3(u_int a[]) { return  a[2] ^ (a[1] | a[3]);          }

typedef u_int (*_dgst_f)(u_int a[]);


u_int *cal_table(u_int *k)
{
    double _p = pow(2.0, 32);
    for(int i = 0; i < 64; i++) {
        double _s = fabs(sin(1.0 + i));
        k[i] = (u_int)(_s * _p);
    }
    return k;
}

u_int rol(u_int r, short n)
{
    u_int mask_1 = (1 << n) - 1;
    return ((r >> (32 - n)) & mask_1) | (( r << n) & ~mask_1);
}

u_int* md5_hash(const char* mesg, int mlen)
{
    static _dgst_arr h0 = { 0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476 };
	static _dgst_f ff[] = { &_f0, &_f1, &_f2, &_f3 };
	static short M[] = { 1, 5, 3, 7 };
	static short O[] = { 0, 1, 5, 0 };
	static short rot0[] = { 7, 12, 17, 22 };
	static short rot1[] = { 5, 9, 14, 20 };
	static short rot2[] = { 4, 11, 16, 23 };
	static short rot3[] = { 6, 10, 15, 21 };
	static short *rots[] = { rot0, rot1, rot2, rot3 };
	static unsigned kspace[64];
	static unsigned *k;

	static _dgst_arr h;
	_dgst_arr abcd;
	_dgst_f fctn;
	short m, o, g;
	unsigned f;
	short *rotn;
	union {
		unsigned w[16];
		char     b[64];
	}mm;
	int os = 0;
	int grp, grps, q, p;
	unsigned char *msg2;

	if (k == NULL) k = cal_table(kspace);

	for (q = 0; q<4; q++) h[q] = h0[q];

	{
		grps = 1 + (mlen + 8) / 64;
		msg2 = (unsigned char*)malloc(64 * grps);
		memcpy(msg2, mesg, mlen);
		msg2[mlen] = (unsigned char)0x80;
		q = mlen + 1;
		while (q < 64 * grps) { msg2[q] = 0; q++; }
		{
			md5_union_t u;
			u.w = 8 * mlen;
			q -= 8;
			memcpy(msg2 + q, &u.w, 4);
		}
	}

	for (grp = 0; grp<grps; grp++)
	{
		memcpy(mm.b, msg2 + os, 64);
		for (q = 0; q<4; q++) abcd[q] = h[q];
		for (p = 0; p<4; p++) {
			fctn = ff[p];
			rotn = rots[p];
			m = M[p]; o = O[p];
			for (q = 0; q<16; q++) {
				g = (m*q + o) % 16;
				f = abcd[1] + rol(abcd[0] + fctn(abcd) + k[q + 16 * p] + mm.w[g], rotn[q % 4]);

				abcd[0] = abcd[3];
				abcd[3] = abcd[2];
				abcd[2] = abcd[1];
				abcd[1] = f;
			}
		}
		for (p = 0; p<4; p++)
			h[p] += abcd[p];
		os += 64;
	}
	return h;
}

std::string
libapp::hash(std::string fname)
{
    std::ifstream file(fname);
    std::string it((std::istreambuf_iterator<char>(file)),
                    std::istreambuf_iterator<char>());

    char str[50];
    strcpy(str, "");
    u_int *d = md5_hash(it.c_str(), it.size());
    md5_union_t u;
    for(int j = 0; j < 4; j++) {
        u.w = d[j];
        char s[8];
        sprintf(s, "%02X%02X%02X%02X", u.b[0], u.b[1], u.b[2], u.b[3]);
        strcat(str, s);
    }

    return string(str);

}

size_t write_data(void* ptr, size_t size, size_t nmemb, FILE* fptr)
{
    return fwrite(ptr, size, nmemb, fptr);
}

err::obj
ctl::obj::download_file(const std::string& url, const std::string& file, bool progress, bool debug)
{
    CURL* curl;
    FILE* fp;
    CURLcode resp;
    curl = curl_easy_init();
    if (curl) {
        fp = fopen(file.c_str(), "wb");
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        if (progress) {
            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        }
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        resp = curl_easy_perform(curl);
        fclose(fp);
        long respcode;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &respcode);
        
        if (resp != CURLE_OK) {
            curl_easy_cleanup(curl);
            return err::obj(respcode, " failed to download file from url " + url);
        } else {
            curl_easy_cleanup(curl);
            return err::obj(respcode);
        }
        curl_easy_cleanup(curl);

    }
    return err::obj(0x12, "failed to init curl, download failed");
}

err::obj
ctl::obj::sync_modules(bool debug)
{
    for(auto a : modules)
    {
        io::process("checking updates for ",a.first);
        auto s = a.second(config);
        auto e = s->Sync(config, debug);
        if (e.status() != 0) {
            io::error("Status Code: ",e.status(), " Message: ", e.mesg());
        }
        delete s;
    }
    return err::obj(0);
}