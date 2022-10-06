#define f1 (1<<14)
//f는 17.14 방식으로 표현한 1 값. p.q방식일때 2**q값이므로 14만큼 shift.

typedef int fixed_t;

fixed_t convert_i2f(int n);
int convert_f2i_round(fixed_t x); // 반올림
int convert_f2i_round_down(fixed_t x); // 버림
fixed_t add_f(fixed_t x, fixed_t y);
fixed_t sub_f(fixed_t x, fixed_t y);
fixed_t add_inf(int n, fixed_t x);
fixed_t sub_inf(int n, fixed_t x);
fixed_t mul_f(fixed_t x, fixed_t y);
fixed_t mul_inf(int n, fixed_t x);
fixed_t div_xbn(fixed_t x, int n);
fixed_t div_xby(fixed_t x, fixed_t y);


int convert_i2f(int n)
{
    return n*f1;
}

int convert_f2i_round(fixed_t x)
{
    if(x>=0) return (x+f1/2)/f1;
    else return (x-f1/2)/f1;
}

int convert_f2i_round_down(fixed_t x)
{
    return x / f1;
}

fixed_t add_f(fixed_t x, fixed_t y)
{
    return x + y;
}

fixed_t sub_f(fixed_t x, fixed_t y)
{
    return x - y;
}

fixed_t add_inf(int n, fixed_t x)
{
    return n*f1 + x;
}

fixed_t sub_inf(int n, fixed_t x)
{
    return n*f1 - x;
}

fixed_t mul_f(fixed_t x, fixed_t y)
{
    return ((int64_t)x) * y / f1;
}

fixed_t mul_inf(int n, fixed_t x)
{
    return x*n;
}
fixed_t div_xbn(fixed_t x, int n)
{
    return x/n;
}
fixed_t div_xby(fixed_t x, fixed_t y)
{
    return ((int64_t)x)*f1/y;
}