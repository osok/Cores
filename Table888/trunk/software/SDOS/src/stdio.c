//#include "c:\AtlysCores\RTF65000\trunk\software\stdio.h"

pascal void putch(char ch)
{
	asm {
		lw		r1,32[bp]
		jsr		($8028)
	}
}

pascal void putnum(int num, int wid, char sep)
{
	int n, m10;
	char sign;
	char numwka[20];

	if (wid < 0 or wid > 200)	// take care of nutty parameter
		wid = 0;
	sign = num < 0 ? '-' : '+';
	if (num < 0) num = -num;
	n = 0;
	do {
		if ((n % 4)==3 && sep) {
			numwka[n]=sep;
			n++;
		}
		m10 = num % 10;
		if (m10 > 9 or m10 < 0)
			printf("moderr ");
		numwka[n] = m10 + '0';
		num = num / 10;
		n++;
	} until (num == 0 or n > 18);
	if (sign=='-') {
		numwka[n] = sign;
		n++;
	}
	for (; n < wid; wid--)
		putch(' ');
	while (n > 0) {
		--n;
		putch(numwka[n]);
	}
}

pascal void puthexnum(int num, int wid)
{
	int n, m;
	char sign;
	char numwka[20];

	if (wid < 0 or wid > 200)	// take care of nutty parameter
		wid = 0;
	n = 0;
	sign = num < 0 ? '-' : '+';
	if (num < 0) num = -num;
	do {
		m = num & 15;
		if (m < 10)
			numwka[n] = m + '0';
		else
			numwka[n] = m + 'A'-10;
		num = num >> 4;
		n++;
	}
	while (num != 0);
	if (sign=='-') {
		numwka[n] = sign;
		n++;
	}
	while (n < wid) {
		putch(' ');
		wid--;
	}
	while (n > 0) {
		--n;
		putch(numwka[n]);
	}
}

pascal int putstr(char *p, int maxchars)
{
	char *q;

	for (q = p; *p && maxchars > 0; p++, maxchars--)
		putch(*p);
	return p-q;
}

naked int getcharNoWait()
{
	asm {
		jmp		($8018)
	}
}

int (getchar)()
{
	int ch;

	do {
		ch = getcharNoWait();
	}
	while (ch==-1);
	return ch & 255;
}


int printf(char *p, ...)
{
	int *q;
	int fmtwidth;
	int maxwidth;
	int wd;
	q = &p;

	for (; *p; p++) {
		if (*p=='%') {
			fmtwidth = 0;
			maxwidth = 65535;
			p++;
j1:
			switch(*p) {
			case '%':
				putch('%');
				break;
			case 'c':
				q++;
				putch(*q);
				break;
			case 'd':
				q++;
				putnum(*q,fmtwidth,0);
				break;
			case 'x':
				q++;
				puthexnum(*q,fmtwidth);
				break;
			case 's':
				q++;
				wd = putstr(*q,maxwidth);
				//while (wd < fmtwidth) {
				//	putch(' ');
				//	wd++;
				//}
				break;
			// width specification
			case '0','1','2','3','4','5','6','7','8','9':
				fmtwidth = *p - '0';
				p++;
				while (isdigit(*p)) { 
					fmtwidth *= 10;
					fmtwidth += *p - '0';
					p++;
				}
				if (*p != '.')
					goto j1;
			case '.':
				p++;
				if (!isdigit(*p))
					goto j1;
				maxwidth = *p - '0';
				p++;
				while (isdigit(*p)) {
					maxwidth *= 10;
					maxwidth += *p - '0';
					p++;
				}
				goto j1;
			}
		}
		else
			putch(*p);
	}
}
