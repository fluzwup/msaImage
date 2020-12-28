
void RGBtoHSV(unsigned char R, unsigned char G, unsigned char B, unsigned char &H, unsigned char &S, unsigned char &V);
void HSVtoRGB(unsigned char H, unsigned char S, unsigned char V, unsigned char &R, unsigned char &G, unsigned char &B);
void YIQtoRGB(unsigned char Y, unsigned char I, unsigned char Q, unsigned char &R, unsigned char &G, unsigned char &B);
void RGBtoYIQ(unsigned char R, unsigned char G, unsigned char B, unsigned char &Y, unsigned char &I, unsigned char &Q);				  
void RGBtoYCbCr(unsigned char R, unsigned char G, unsigned char B, unsigned char *Y, unsigned char *Cb, unsigned char *Cr); 
void YCbCrtoRGB(unsigned char Y, unsigned char Cb, unsigned char Cr, unsigned char &R, unsigned char &G, unsigned char &B);

