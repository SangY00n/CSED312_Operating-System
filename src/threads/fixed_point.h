#define f 1 << 14
//f는 17.14 방식으로 표현한 1 값. p.q방식일때 2**q값이므로 14만큼 shift.

int convert_i2f(int n);
int convert_f2i_round(int x); //반올림
int convert_f2i_nearest(int x); //nearest
int add_f(int x, int y);
int sub_f(int x, int y);
int add_inf(int n, int x);
int sub_inf(int n, int x);
int mul_f(int x, int y);
int mul_inf(int x, int n);
int div_xby(int x, int y);
int div_xbn(int x, int n);

int convert_i2f(int n)
{
    return n*f;
}

int convert_f2i_round(int x)
{
    return x / f;
}

int convert_f2i_nearest(int x)
{
    if(x>=0) return x+f/2;
    else return x-f/2;
}

int add_f(int x, int y)
{
    return x + y;
}

int sub_f(int x, int y)
{
    return x - y;
}

int add_inf(int n, int x)
{
    return n*f + x;
}

int sub_inf(int n, int x)
{
    return n*f - x;
}

int mul_f(int x, int y)
{
    return ((int64_t)x) * y / f;
}

int mul_inf(int n, int x)
{
    return x*n;
}
int div_xbn(int x, int n)
{
    return x/n;
}
int div_xby(int x, int y)
{
    return ((int64_t)x)*f/y;
}