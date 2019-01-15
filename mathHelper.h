// See: http://blog.dreasgrech.com/2009/11/greatest-common-divisor-and-least.html

int gcd(int x, int y) {
    /*;
        a = qb + r,  0 <= r < b

        a => dividend, q => quotient, b => divisor, r => remainder
    */
    if (x == y) {
        return x /*or y*/;
    }

    int dividend = x, divisor = y, remainder = 0, quotient = 0;

    do {
        remainder = dividend % divisor;
        quotient = dividend / divisor;

        if(remainder) {
            dividend = divisor;
            divisor = remainder;
        }
    }
    while(remainder);

    return divisor;
}

int lcm(int x, int y) {
    /*
        lcm(x,y) = (x * y) / gcd(x,y)
    */
    return x == y ? x /*or y*/ : (x * y) / gcd(x,y);
}
