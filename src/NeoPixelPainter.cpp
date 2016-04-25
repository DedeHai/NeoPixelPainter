/**
* Copyright (c) D. Schneider, 2016 <daedae@gmx.ch>
*
* This program is free software: you can redistribute it and/or modify it under
* the terms of the GNU General Public License as published by the Free Software
* Foundation, either version 3 of the License, or (at your option) any later
* version.
* 
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
* FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
* details.
* 
* You should have received a copy of the GNU General Public License along with
* this program. If not, see <http://www.gnu.org/licenses/>.
*/


#include "NeoPixelPainter.h"


NeoPixelPainterCanvas::NeoPixelPainterCanvas(Adafruit_NeoPixel* NeoPixels)
{
	//allocate memory for a canvas, add it to the canvas list and return the pointer to the new canvas
	_LED = NeoPixels;
	_canvas = (hsvcanvas*)calloc(_LED->numPixels(),sizeof(hsvcanvas)); //create an array of canvas pixels

}

NeoPixelPainterCanvas::~NeoPixelPainterCanvas(void)
{
	if(_canvas) free(_canvas); 
}

bool NeoPixelPainterCanvas::isvalid(void)
{
	if(_canvas) return true;
	else return false;
}

void  NeoPixelPainterCanvas::clear() //clear all hsv fade pixels on the canvas
{
	memset(_canvas, 0, _LED->numPixels()*sizeof(hsvcanvas));
}

//update & transfer the hsv canvas to the LEDs
void NeoPixelPainterCanvas::transfer(void){

	if(isvalid()==false) return; //canvas painting array was not properly allocated
	int i;
	for (i = 0; i < _LED->numPixels(); i++)
	{
		addColorHSV(i,  _canvas[i].h, _canvas[i].s, _canvas[i].v);

		if (_canvas[i].fadespeed > 0)
		{
			//do the fading
			//the fadespeed is divided into a value that is added and a time interval
			//this allows for very slow as well as very fast fading using only one variable
			int8_t addvalue = 1;
			uint16_t interval = 800 / _canvas[i].fadespeed; //means 160.0/fadespeed with fixed decimal to avoid floats

			//formula determines the number of intervals it takes for the calculated integer interval to be
			//deviating more than 1 if a float interval was used (formula in floats: 1/(1/floatinterval-1/int_interval) )
			//on intervals lower than one, make the interval 1 and work on adder only

			if (interval > 10) //interval > 1.0
			{
				uint8_t subinterval = interval % 10;
				interval = interval / 10;
				if (subinterval > 0)
				{

					if (_speedcounter % ((interval * interval * 10 + interval * subinterval) / subinterval) == 0)  _canvas[i].skipfadeupdate = 1;

				}
			}
			else //on faster values
			{
				interval = 1;
				addvalue = (_canvas[i].fadespeed + (_canvas[i].fadespeed - 80)) / 8;
				uint8_t subvalue = addvalue % 10;
				addvalue = addvalue / 10;
				if (subvalue > 0)
				{
					if (subvalue >= 5)
					{
						addvalue++; //round up the addvalue
						if (_speedcounter %  (10 / (10 - subvalue)) == 0) addvalue = 1;
					}
					else
					{
						if (_speedcounter % (10 / subvalue) == 0) addvalue++;
					}
				}
			}


			if ((_speedcounter % interval) == 0 )
			{
				if (_canvas[i].skipfadeupdate == 0)
				{
					//fade value
					if (_canvas[i].fadevalue_in)
					{
						if (_canvas[i].fadevalue - addvalue <= _canvas[i].v)
						{
							_canvas[i].v = _canvas[i].fadevalue;
							_canvas[i].fadevalue_in = 0; //finished with fade in
						}
						else _canvas[i].v += addvalue;
					}
					else if (_canvas[i].fadevalue_out) //only fade out if fade in is finished
					{
						if (_canvas[i].v <= addvalue)
						{
							_canvas[i].v = 0;
							_canvas[i].fadevalue_out = 0; //prevents this being executed over and over again after fadout
						}
						else     _canvas[i].v -= addvalue;
					}

					//fade hue
					if (_canvas[i].fadehue_up)
					{
						if (_canvas[i].fadevalue == _canvas[i].h)
						{
							_canvas[i].fadehue_up = 0; //finished with fade in
						}
						else _canvas[i].h++;
					}
					else if (_canvas[i].fadehue_down)
					{
						if (_canvas[i].fadevalue == _canvas[i].h)
						{
							_canvas[i].fadehue_down = 0; //prevents this if being executed over and over again after fadout
						}
						else     _canvas[i].h--;
					}

					//fade saturation
					if (_canvas[i].fadesaturation_in)
					{
						if (_canvas[i].fadevalue == _canvas[i].s)
						{
							_canvas[i].fadesaturation_in = 0; //finished with fade in
						}
						else _canvas[i].s++;
					}
					else if (_canvas[i].fadesaturation_out)
					{
						if (_canvas[i].fadevalue == _canvas[i].s)
						{
							_canvas[i].fadesaturation_out = 0; //prevents this if being executed over and over again after fadout
						}
						else     _canvas[i].s--;
					}

				}
				else _canvas[i].skipfadeupdate = 0; //fadeupdate skipped, continue normally
			}
		}
	}
	_speedcounter++;
}


void NeoPixelPainterCanvas::addColorRGB(int index, RGB color)
{
	RGB nowcolor;
	uint32_t temp = _LED->getPixelColor(index);
	nowcolor.r = (temp >> 16) & 0xFF;
	nowcolor.g = (temp >> 8) & 0xFF;
	nowcolor.b = temp & 0xFF;

	nowcolor.r += color.r;
	nowcolor.g += color.g;
	nowcolor.b += color.b;

	//check for overflow, set to max if overflown

	if (nowcolor.r < color.r) nowcolor.r = 255;
	if (nowcolor.g < color.g) nowcolor.g = 255;
	if (nowcolor.b < color.b) nowcolor.b = 255;

	_LED->setPixelColor(index, nowcolor.r, nowcolor.g, nowcolor.b);
}


void NeoPixelPainterCanvas::addColorHSV(int index, uint8_t H, uint8_t S, uint8_t V)
{
	RGB color;
	color = HSVtoRGB(H, S, V);
	addColorRGB(index, color);
}




//HSV to RGB using only integers (error as opposed to using floats is negligeable, maximum 2LSB)
RGB NeoPixelPainterCanvas::HSVtoRGB(uint8_t H, uint8_t S, uint8_t V) {
	RGB returncolor;
	uint16_t p, q, t, i, f,h,s,v;
	h=H; s= S; v = V;
	if (s == 0) {
		returncolor.r = returncolor.g = returncolor.b = v;
	} else {
		h = h << 4;
		i = h / 680;
		f = (h % 680) >> 4;
		p = (v * (255 - s))  >> 8;
		q = (v * ((10710 - (s * f)) / 42))  >> 8;
		t = (v * ((10710 - (s * (42 - f))) / 42)) >> 8;

		switch (i) {
		case 1:
			returncolor.r = q;
			returncolor.g = v;
			returncolor.b = p;
			break;
		case 2:
			returncolor.r = p;
			returncolor.g = v;
			returncolor.b = t;
			break;
		case 3:
			returncolor.r = p;
			returncolor.g = q;
			returncolor.b = v;
			break;
		case 4:
			returncolor.r = t;
			returncolor.g = p;
			returncolor.b = v;
			break;
		case 5:
			returncolor.r = v;
			returncolor.g = p;
			returncolor.b = q;
			break;
		default:  //in case i > 5 or i == 0
			returncolor.r = v;
			returncolor.g = t;
			returncolor.b = p;
			break;
		}
	}

	return returncolor;

}













//a brush paints a hsvfade-pixel-canvas in a color. also supports fade-in
//it moves according to its current speed (negative moves backwards)
//it only paints a pixel once (without painting any other pixel that is), even when called multiple times on the same pixel 
//to paint the same pixel (and only that one) multiple times, use the moveTo() function before painting




NeoPixelPainterBrush::NeoPixelPainterBrush(NeoPixelPainterCanvas* hsv_canvas)
{
	_brush = (brush*)calloc(1,sizeof(brush)); //create an array of canvas pixels
	_canvastopaint = hsv_canvas;
	if(isvalid())
	{
		moveTo(0); //move painter to first pixel
		_brush->speedlimit = 0x6FFF; //do not limit speed initially
	} 

}

NeoPixelPainterBrush::~NeoPixelPainterBrush(void)
{
	if(_brush) free(_brush); //todo: is this a good idea???
}

bool NeoPixelPainterBrush::isvalid(void)
{
	if(_brush!= NULL && _canvastopaint->isvalid()) return true;
	else return false;
}

//update and paint to the canvas
void NeoPixelPainterBrush::paint(void){

	if(_canvastopaint == NULL || _brush == NULL) return; //make sure we got a valid pointer

	if (((_brush->i) >> SPEEDSHIFT) != _brush->position)
	{
		_brush->position = _brush->i >> SPEEDSHIFT;

		//determine which value(s) to fade and which to paint directly
		//note: only one fade-to value can be saved. if multiple are fadings are used,
		//the highest priority value is painted. priority is h,s,v (h being the highest priority)
		//so a fade of value and hue to a value of 100 and a hue of 50 will fade the value also to 50
		//fadeout can always be used becaus it fades to 0
		//to avoid this mess, more values would have to be stored in the _canvastopaint, using up too much ram (on higher ram chips, this may be added)

		_canvastopaint->_canvas[_brush->position].fadespeed = _brush->fadespeed;
		if (_brush->fadevalue_in == 0)  _canvastopaint->_canvas[_brush->position].v = _brush->value; //no fade? -> set the value
		else  _canvastopaint->_canvas[_brush->position].fadevalue = _brush->value; //use fade? set the fadevalue
		
		if ((_brush->fadesaturation_in || _brush->fadesaturation_out) == 0)  _canvastopaint->_canvas[_brush->position].s = _brush->saturation; //no fade? -> set the value
		else  _canvastopaint->_canvas[_brush->position].fadevalue = _brush->saturation; //use fade? set the fadevalue
		
		if ((_brush->fadehue_near || _brush->fadehue_far) == 0)  _canvastopaint->_canvas[_brush->position].h = _brush->hue; //no fade? -> set the value
		else  
		{
			_canvastopaint->_canvas[_brush->position].fadevalue = _brush->hue; //use fade? set the fadevalue
			//determine hue fade direction and set it

			bool count_up = true; //clockwise (count up) as default
			bool nochange = false; //do not change fading if painting the same hue again
			//find the nearest direction towards the new hue:
			if(_brush->hue > _canvastopaint->_canvas[_brush->position].h)
			{
				if(_brush->hue - _canvastopaint->_canvas[_brush->position].h > 128) 
				{
					count_up = false; //nearest path is counter clockwise

				}
			}
			else if(_brush->hue < _canvastopaint->_canvas[_brush->position].h)
			{
				if(_canvastopaint->_canvas[_brush->position].h - _brush->hue < 128) 
				{
					count_up = false; //nearest path is counter clockwise
				}
			}
			else //same color again, do not set any fading direction
			{
				nochange = true;
			}

			if(_brush->fadehue_far) count_up = !count_up; //invert direction if fading along the far path

			if(nochange == false)
			{
				if(count_up)
				{
					_canvastopaint->_canvas[_brush->position].fadehue_up = 1;
				}      
				else
				{
					_canvastopaint->_canvas[_brush->position].fadehue_down = 1;
				}
			}     

		}
		
		//paint the flags
		_canvastopaint->_canvas[_brush->position].fadevalue_in = _brush->fadevalue_in;
		_canvastopaint->_canvas[_brush->position].fadevalue_out = _brush->fadevalue_out;
		_canvastopaint->_canvas[_brush->position].fadesaturation_in = _brush->fadesaturation_in;
		_canvastopaint->_canvas[_brush->position].fadesaturation_out = _brush->fadesaturation_out;
		
	}


	//check if speed is within the limit
	//  if ( _brush->speed >  _brush->speedlimit)  _brush->speed = _brush->speedlimit;
	//  if ( _brush->speed < -_brush->speedlimit)  _brush->speed = -_brush->speedlimit;

	//apply speed
	_brush->i +=  _brush->speed;

	//jump or bounce at end of strip:
	if (_brush->i < 0)
	{
		if (_brush->bounce)
		{
			_brush->i = 0;
			_brush->speed = -_brush->speed;
		}
		else
		{
			_brush->i = ((int32_t)(_canvastopaint->_LED->numPixels() - 1)) << SPEEDSHIFT;
		}
	}
	else if (((_brush->i) >> SPEEDSHIFT) >= _canvastopaint->_LED->numPixels())
	{
		if (_brush->bounce)
		{
			_brush->i = ((int32_t)(_canvastopaint->_LED->numPixels() - 1)) << SPEEDSHIFT;
			_brush->speed = -_brush->speed;
		}
		else
		{
			_brush->i = 0;
		}
	}
}


void NeoPixelPainterBrush::moveTo(uint16_t position)
{
	if(position >=  _canvastopaint->_LED->numPixels())
	{
		position = _canvastopaint->_LED->numPixels() - 1;
	}
	_brush->i = ((int32_t)position<<SPEEDSHIFT);
	_brush->position = 0xFFFF; //invalidate the current position marking so the pixel always gets updated
}
int16_t NeoPixelPainterBrush::getPosition(void)
{
	return _brush->i>>SPEEDSHIFT;
}

void NeoPixelPainterBrush::setSpeed(int16_t speed)
{
	_brush->speed = speed;
}
uint16_t NeoPixelPainterBrush::getSpeed(void)
{
	return _brush->speed;
}
void NeoPixelPainterBrush::setSpeedlimit(int16_t limit)
{
	_brush->speedlimit = limit;
}

void NeoPixelPainterBrush::setColor(HSV color)
{
	_brush->hue = color.h;
	_brush->saturation = color.s;
	_brush->value = color.v;
}

HSV NeoPixelPainterBrush::getColor(void)
{
	HSV returncolor;
	returncolor.h = _brush->hue;
	returncolor.s =_brush->saturation;
	returncolor.v =_brush->value;
	return returncolor;
}

void  NeoPixelPainterBrush::setFadeSpeed(uint8_t fadeSpeed)
{
	_brush->fadespeed = fadeSpeed;
}
uint8_t NeoPixelPainterBrush::getFadeSpeed(void)
{
	return _brush->fadespeed;
}
void NeoPixelPainterBrush::setFadeout(bool value)
{
	_brush->fadevalue_out = value;
}

void NeoPixelPainterBrush::setFadein(bool value)
{
	_brush->fadevalue_in = value;
}


void NeoPixelPainterBrush::setFadeHueNear(bool value)
{
	_brush->fadehue_near = value;
}

void NeoPixelPainterBrush::setFadeHueFar(bool value)
{
	_brush->fadehue_far = value;
}

void NeoPixelPainterBrush::setFadeSaturation_in(bool value)
{
	_brush->fadesaturation_in = value;
}

void NeoPixelPainterBrush::setFadeSaturation_out(bool value)
{
	_brush->fadesaturation_out = value;
}

void NeoPixelPainterBrush::setBounce(bool value)
{
	_brush->bounce = value;
}




/*
stuff needed:

also: add option (define) to use much faster 8bit hsv to rgb in case anyone wants that.
does it makes transitions noticeably less smooth? 
*/




