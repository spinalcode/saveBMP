#include <Pokitto.h>
#include "sqrt_table.h"
#include <File>

File bmpsavefile;
const char* bmp_filename = "/bitmap.bmp";
bool savingBMP = false;
bool bmpFileOpened = false;
#define lineSize 440 /*((220*2)+((4-(220*2)%4)%4))*/
int bmpDataOffset = 66+(176*lineSize);

#define ABS(x) ((x)<0 ? -(x) : (x))

using PC = Pokitto::Core;
using PD = Pokitto::Display;
using PB = Pokitto::Buttons;

int width=220;
int height=176;
long int frameCounter;

const int NUM_CIRCLES = 4;
struct circle{
    int x;
    int y;
    int r;
    int vx;
    int vy;
    int cr;
    int cg;
    int cb;
}; 

static circle c[NUM_CIRCLES];
int RandMinMax(int min, int max){
    return rand() % max + min;
}

void createTable(){
    int t=0;
    for(int y=0; y<176; y++){
        for(int x=0; x<256; x++){
		    t=(9216 / (sqrt(x*x + y*y) * 8));
            if (t > 255) t = 255;
        }
    }		   
}

inline void myBGFiller(std::uint8_t* line, std::uint32_t y, bool skip){

    // Using the 16bit mode cheat, each pixel is numbered from 0 to 220
    // Then we change the palette entry rather than the pixel number.
    if(y==0){
        for(uint32_t x=0; x<220; ++x){
            line[x]=x;
        }        
    }

    int i=0;
    int x1,y1,temp;
    uint32_t red=0,green=0,blue = 0;
    int x=0;
    auto lineP = &Pokitto::Display::palette[0];

    int ty[NUM_CIRCLES];
    for (int i = 0; i < NUM_CIRCLES; ++i) {
        int y1 = c[i].y-y;
        ty[i] = 256 * ABS(y1);
    }
    
    for (int x = 220; x; --x) {
        uint32_t red = 0, green = 0, blue = 0;
        int temp=0;
        for (int i = 0; i < NUM_CIRCLES; ++i) {
            int x1 = c[i].x-x;
            temp = table[ABS(x1) + ty[i]];
            red   += temp * c[i].cr;
            green += temp * c[i].cg;
            blue  += temp * c[i].cb;
        }

        if(red>255)red=255;
        if(green>255)green=255;
        if(blue>255)blue=255;
        *lineP++ = ((red&248)<<8) | ((green&252)<<3) | (blue>>3); // is this any faster?
    } 

}


void saveBMP(std::uint8_t* line, std::uint32_t y, bool skip){

    if(savingBMP){
        char w=220, h=176;

        if(y==0){
            int filesize = 66 + 2*w*h;  //w is your image width, h is image height, both int
            unsigned char bmpfileheader[14] = {'B','M', 0,0,0,0, 0,0, 0,0, 66,0,0,0}; // start the pixel data at 66 instead of the usual 54
            unsigned char bmpinfoheader[40] = {40,0,0,0, 0,0,0,0, 0,0,0,0, 1,0, 16,0,3}; // 16bpp
            uint32_t bmppixelformat[] = {0xF800, 0x7E0, 0x1F}; // tell the file that 16bit data is RGB565
            
            bmpfileheader[ 2] = (unsigned char)(filesize    );
            bmpfileheader[ 3] = (unsigned char)(filesize>> 8);
            bmpfileheader[ 4] = (unsigned char)(filesize>>16);
            bmpfileheader[ 5] = (unsigned char)(filesize>>24);
            
            bmpinfoheader[ 4] = (unsigned char)(       w    );
            bmpinfoheader[ 5] = (unsigned char)(       w>> 8);
            bmpinfoheader[ 6] = (unsigned char)(       w>>16);
            bmpinfoheader[ 7] = (unsigned char)(       w>>24);
            bmpinfoheader[ 8] = (unsigned char)(       h    );
            bmpinfoheader[ 9] = (unsigned char)(       h>> 8);
            bmpinfoheader[10] = (unsigned char)(       h>>16);
            bmpinfoheader[11] = (unsigned char)(       h>>24);
            
            if(bmpsavefile.openRW(bmp_filename,1,1)){
                bmpFileOpened = true;
                bmpsavefile.write(bmpfileheader,14);
                bmpsavefile.write(bmpinfoheader,40);
                bmpsavefile.write(bmppixelformat,12);
            }else{
                bmpFileOpened = false;
            }
            
            bmpDataOffset = 66+(175*lineSize);

        }
        
        if(bmpFileOpened == true){
            bmpsavefile.seek(bmpDataOffset); // can we seek a file for WRITING?
            bmpDataOffset -= lineSize;
            // dump each palette line directly to the file, as the format is RGB565
            bmpsavefile.write(&Pokitto::Display::palette[0] , 440);

            if(y==175){
                savingBMP = false;
                bmpsavefile.close();
            }
        }
    }

}



void move_circles(){
    for (int i = 0; i < NUM_CIRCLES; i++) {
    
        c[i].x += c[i].vx;
        c[i].y += c[i].vy;
    
        if (c[i].x - c[i].r < 0) {
            c[i].vx = +abs(c[i].vx);
        }
        if (c[i].x + c[i].r > width) {
            c[i].vx = -abs(c[i].vx);
        }
        if (c[i].y - c[i].r < 0) {
            c[i].vy = +abs(c[i].vy);
        }
        if (c[i].y + c[i].r > height) {
            c[i].vy = -abs(c[i].vy);
        }
    }
}

int main(){

	PC::begin();

    Pokitto::Display::lineFillers[0] = myBGFiller;
    Pokitto::Display::lineFillers[1] = saveBMP;

    uint32_t frameCount=0;

    srand(PC::getTime());
    for (int i = 0; i < NUM_CIRCLES; i++) {
        c[i].x = RandMinMax(0,110) + 110 * (i%2);
        c[i].y = RandMinMax(0,88) + 88 * (i%2);
        c[i].r = RandMinMax(5, 15);
        c[i].vx = RandMinMax(-5,5);
        c[i].vy = RandMinMax(-5,5);
        switch(i%6){
            case 0:
                c[i].cr = 2;
                c[i].cg = 1;
                c[i].cb = 1;
            break;
            case 1:
                c[i].cr = 1;
                c[i].cg = 2;
                c[i].cb = 1;
            break;
            case 2:
                c[i].cr = 1;
                c[i].cg = 1;
                c[i].cb = 2;
            break;
            case 3:
                c[i].cr = 2;
                c[i].cg = 2;
                c[i].cb = 1;
            break;
            case 4:
                c[i].cr = 1;
                c[i].cg = 2;
                c[i].cb = 2;
            break;
            case 5:
                c[i].cr = 2;
                c[i].cg = 1;
                c[i].cb = 2;
            break;
        }
    }


    //createTable(); // used to create the lookup table
    while (1) {
        if(!PC::update())continue;
        move_circles();
        
        // Flash button
        #ifndef POK_SIM
        long unsigned int pins = (((volatile uint32_t *) 0xA0000000)[0]);
        if((pins >> 8 & 1) == 0){
            // wait until flash button is released before continuing
            while(((((volatile uint32_t *) 0xA0000000)[0]) >> 8 & 1) == 0){PC::update();} // wait for button release
            savingBMP = true;
        }
        #endif
        
    }

    return 0;
}