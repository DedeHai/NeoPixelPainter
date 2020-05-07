/**
   This example shows how to use one stripe for 2 different segment animations
*/

//let's assume, our strip is 30 LEDs long, devide it in two parts:
#define NUMBEROFPIXELS_LEFT 15 //Number of LEDs on the first part of strip
#define NUMBEROFPIXELS_RIGHT 15 //Number of LEDs on the second part of strip


#define PIXELPIN 2 //Pin where WS281X LED strip data-line is connected

#include "Arduino.h"
#include <Adafruit_NeoPixel.h>
#include <NeoPixelPainter.h>


Adafruit_NeoPixel neopixels = Adafruit_NeoPixel(NUMBEROFPIXELS_LEFT + NUMBEROFPIXELS_RIGHT, PIXELPIN, NEO_GRB + NEO_KHZ800);//

//create one canvas and one brush for each part
NeoPixelPainterCanvas pixelcanvasleft = NeoPixelPainterCanvas(&neopixels, NUMBEROFPIXELS_LEFT); //create canvas, linked to the neopixels (must be created before the brush)
NeoPixelPainterBrush pixelbrushleft = NeoPixelPainterBrush(&pixelcanvasleft); //crete brush, linked to the canvas to paint to

NeoPixelPainterCanvas pixelcanvasright = NeoPixelPainterCanvas(&neopixels, NUMBEROFPIXELS_RIGHT); //create canvas, linked to the neopixels (must be created before the brush)
NeoPixelPainterBrush pixelbrushright = NeoPixelPainterBrush(&pixelcanvasright); //crete brush, linked to the canvas to paint to

HSV brushcolorLeft;
HSV brushcolorRight;
RGB rgbBrushColorLeft;
bool state = false; //state of the left part of strip: true = on, false = off
unsigned long lastMillis = 0;
bool delayTimerOn = false;

//this function sets the color of each LED on the left part. LED numbers 0..NUMBEROFPIXELS_LEFT-1 on real strip
void addColorRGBleft(int index, RGB color){
    neopixels.setPixelColor(index, color.r, color.g, color.b);
}
//this function sets the color of each LED on the right part. LED numbers NUMBEROFPIXELS_LEFT..NUMBEROFPIXELS_RIGHT-1 on real strip
//index begins always at 0 and ends at NUMBEROFPIXELS_*-1
//for the right part we have to set LED position with offset of NUMBEROFPIXELS_LEFT
void addColorRGBright(int index, RGB color){
    neopixels.setPixelColor(index + NUMBEROFPIXELS_LEFT, color.r, color.g, color.b);
}

bool isPixelOn(uint16_t index){
    uint32_t pixelColor = neopixels.getPixelColor(index);
    if(pixelColor)
       return true;
    else
       return false;
}

bool compareColors(uint16_t index, RGB color){
      uint32_t pixelColor = neopixels.getPixelColor(index);
   
      if(color.r == ((pixelColor >> 16) & 0xFF) &&
       color.g == ((pixelColor >> 8) & 0xFF) &&
       color.b == (pixelColor & 0xFF))
            return true;
        else
            return false;
}

void setup() {
    neopixels.begin();
    //set user defined AddColorRGB-functions (because of offset)
    pixelcanvasleft.setExtAddColorRGB(addColorRGBleft);
    pixelcanvasright.setExtAddColorRGB(addColorRGBright);
    
    Serial.begin(115200);
    Serial.println(" ");
    Serial.println(F("NeoPixel Painter splitted strip demo"));

    //check if ram allocation of brushes and canvases was successful (painting will not work if unsuccessful, program should still run though)
    //this check is optional but helps to check if something does not work, especially on low ram chips like the Arduino Uno
    if (pixelcanvasleft.isvalid() == false) Serial.println(F("canvasleft allocation problem (out of ram, reduce number of pixels)"));
    else  Serial.println(F("canvasleft allocation ok"));
  
    if (pixelbrushleft.isvalid() == false) Serial.println(F("brushleft allocation problem"));
    else  Serial.println(F("brushleft allocation ok"));

    if (pixelcanvasright.isvalid() == false) Serial.println(F("canvasright allocation problem (out of ram, reduce number of pixels)"));
    else  Serial.println(F("canvasright allocation ok"));
  
    if (pixelbrushright.isvalid() == false) Serial.println(F("brushright allocation problem"));
    else  Serial.println(F("brushright allocation ok"));

    //initialize animation for the left part:
    brushcolorLeft.h = random(255); //set random color
    brushcolorLeft.s = 255; //full color saturation
    brushcolorLeft.v = 50; //about half the full brightness
    rgbBrushColorLeft = pixelcanvasleft.HSVtoRGB(brushcolorLeft.h, brushcolorLeft.s, brushcolorLeft.v); //RGB representation of the brush color
    pixelbrushleft.setSpeed(NUMBEROFPIXELS_LEFT * 5 + random(20)); //movement speed
    pixelbrushleft.setFadeSpeed(NUMBEROFPIXELS_LEFT); //set fading speed
    pixelbrushleft.setColor(brushcolorLeft); //update the color of the brush
    pixelbrushleft.setStop(true); //set stop at the end
    pixelbrushleft.setFadein(true); //do brightness fadeout after painting

    //initialize animation for the right part:
    brushcolorRight.h = 0; //zero is red in HSV. Library uses 0-255 instead of 0-360 for colors (see https://en.wikipedia.org/wiki/HSL_and_HSV)
    brushcolorRight.s = 255; //full color saturation
    brushcolorRight.v = 50; //about half the full brightness
    pixelbrushright.setSpeed(NUMBEROFPIXELS_RIGHT * 5 + random(20)); //movement speed
    pixelbrushright.setFadeSpeed(NUMBEROFPIXELS_RIGHT * 2); //set fading speed
    pixelbrushright.setColor(brushcolorRight); //update the color of the brush
    pixelbrushright.setBounce(true); //bounce the brush when it reaches the end of the strip
    pixelbrushright.setFadein(true); //do brightness fadeout after painting
    pixelbrushright.setFadeout(true); //do brightness fadeout after painting
}

void loop() {
    static uint32_t lastposition = 0;
    if (pixelbrushleft.getPosition() == (NUMBEROFPIXELS_LEFT - 1) && lastposition > 0){
        //if brush color and the color of last pixel are the same and strip was off animation will stop
        //if last pixel is of and strip was on animation will stop also
        if((compareColors(NUMBEROFPIXELS_LEFT - 1, rgbBrushColorLeft) && !state) ||
           (!isPixelOn(NUMBEROFPIXELS_LEFT - 1) && state)){
            Serial.println("Animation is done, change parameters and wait a bit...");
            if(state){
                pixelbrushleft.setFadeout(false); //LED will stay on
                brushcolorLeft.h = random(255); //set new color, because we can
                pixelbrushleft.setColor(brushcolorLeft); //update the color of the brush
                rgbBrushColorLeft = pixelcanvasleft.HSVtoRGB(brushcolorLeft.h, brushcolorLeft.s, brushcolorLeft.v); //RGB representation of the brush color
                state = false;
            }else{
                pixelbrushleft.setFadeout(true); //this will wipe out current color
                state = true;
            }
            brushcolorLeft.h = random(255);

            lastMillis = millis();
            delayTimerOn = true;
         }
    }
    //wait 5 seconds and resume animation
    if(delayTimerOn && millis() - lastMillis >= 5000){
        //we are at the end, brush stays here... to restart animation, move to zero position
        pixelbrushleft.moveTo(0);
        delayTimerOn = false;
        Serial.println("Animation is resumed");
    }
    
    lastposition = pixelbrushleft.getPosition();

    neopixels.clear(); //always need to clear the pixels
  
    pixelbrushleft.paint(); //paint the brush to the canvas (and update the brush, i.e. move it a little)
    pixelcanvasleft.transfer(); //transfer the canvas to the neopixels
    pixelbrushright.paint(); //paint the brush to the canvas (and update the brush, i.e. move it a little)
    pixelcanvasright.transfer(); //transfer the canvas to the neopixels
  
    neopixels.show();
}
