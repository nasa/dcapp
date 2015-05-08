void getbytes(char *in, char *out, int paramsize, int swapflag)
{
    int i;

    if (swapflag)
    {
        for (i=0; i<paramsize; i++) out[i] = in[paramsize-1-i];
    }
    else
    {
        for (i=0; i<paramsize; i++) out[i] = in[i];
    }
}
