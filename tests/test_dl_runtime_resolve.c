#include <stdio.h>
#include <rpc/xdr.h>

int main(int argc, char* argv[])
{
    XDR xdr;
    xdrmem_create(&xdr, NULL, 0, XDR_ENCODE);
    char c = 'L';
    xdr_char(&xdr, &c);
    return 0;
}
