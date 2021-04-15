/*
* Project Name: ERM19264_UC1609
* File: ERM19264_UC1609.cpp
* Description: ERM19264 LCD driven by UC1609C controller soucre file
* Author: Gavin Lyons.
* URL: https://github.com/gavinlyonsrepo/ERM19264_UC1609
*/

#include "ERM19264_UC1609.h"


// Class Constructors // 4 off
// No buffer , Hardware + software SPI
// buffer, Hardware + software SPI 

#ifdef NO_BUFFER
// Hardware SPI
ERM19264_UC1609  :: ERM19264_UC1609(int8_t cd, int8_t rst, int8_t cs) 
{
  _LCD_CD = cd;
  _LCD_RST= rst;
  _LCD_CS = cs;
  _LCD_DIN = -1;   // -1 for din  specify using hardware SPI
  _LCD_SCLK = -1;    // -1 for  sclk specify using hardware SPI
}

// Software SPI
ERM19264_UC1609  :: ERM19264_UC1609(int8_t cd, int8_t rst, int8_t cs, int8_t sclk, int8_t din)
{
  _LCD_CD = cd;
  _LCD_RST= rst;
  _LCD_CS = cs;
  _LCD_DIN = din;  
  _LCD_SCLK = sclk;
  
}

#else

ERM19264_UC1609  :: ERM19264_UC1609(int8_t cd, int8_t rst, int8_t cs) :custom_graphics(LCD_WIDTH, LCD_HEIGHT) 
{
  _LCD_CD = cd;
  _LCD_RST= rst;
  _LCD_CS = cs;
  _LCD_DIN = -1;   // -1 for din and sclk specify using hardware SPI
  _LCD_SCLK = -1;
}

// Software SPI
ERM19264_UC1609  :: ERM19264_UC1609(int8_t cd, int8_t rst, int8_t cs, int8_t sclk, int8_t din) : custom_graphics(LCD_WIDTH, LCD_HEIGHT) 

{
  _LCD_CD = cd;
  _LCD_RST= rst;
  _LCD_CS = cs;
  _LCD_DIN = din;  
  _LCD_SCLK = sclk;
}
#endif

// Desc: begin Method initialise LCD 
// Sets pinmodes and SPI setup
// Param1: VBiasPOT default = 0x49 , range 0x00 to 0xFE
void ERM19264_UC1609::LCDbegin (uint8_t VbiasPOT) 
{
  pinMode(_LCD_CD , OUTPUT);
  pinMode(_LCD_RST, OUTPUT);
  pinMode(_LCD_CS, OUTPUT);
  
  _VbiasPOT  = VbiasPOT;
  if (isHardwareSPI()) 
  {
   SPI.begin();
   //There is a pre-defined macro SPI_HAS_TRANSACTION in SPI library for checking whether the firmware //of the Arduino board supports SPI.beginTransaction().
#ifdef SPI_HAS_TRANSACTION
    {
    SPI.beginTransaction(SPISettings(SPI_FREQ, SPI_DIRECTION, SPI_UC1609_MODE));
   }
#else
    {
        //STM32 blue pill uses this 
        SPI.setClockDivider(SPI_UC1609_CLOCK_DIV); // 72/8 = 9Mhz
      }
#endif

  }else
  {
     // Set software SPI specific pin outputs.
    pinMode(_LCD_DIN, OUTPUT);
    pinMode(_LCD_SCLK, OUTPUT);
  }
  
  LCDinit();
}


// Desc: Called from LCDbegin carries out Power on sequence and register init
// Can be used to reset LCD to default values.
void ERM19264_UC1609::LCDinit()
 {
  delay(UC1609_INIT_DELAY2); //3mS delay, datasheet
  UC1609_CD_SetHigh;
  UC1609_CS_SetHigh;
  LCDReset();

  UC1609_CS_SetLow;

  send_command(UC1609_TEMP_COMP_REG, UC1609_TEMP_COMP_SET); 
  send_command(UC1609_ADDRESS_CONTROL, UC1609_ADDRESS_SET); 
  send_command(UC1609_FRAMERATE_REG, UC1609_FRAMERATE_SET);
  send_command(UC1609_BIAS_RATIO, UC1609_BIAS_RATIO_SET);  
  send_command(UC1609_POWER_CONTROL,  UC1609_PC_SET); 
  delay(UC1609_INIT_DELAY);
  
  send_command(UC1609_GN_PM, 0);
  send_command(UC1609_GN_PM, _VbiasPOT); //  changed by user
  
  send_command(UC1609_DISPLAY_ON, 0x01); // turn on display
  send_command(UC1609_LCD_CONTROL, UC1609_ROTATION_NORMAL); // rotate to normal 
  
  UC1609_CS_SetHigh;
}


// Desc: Sends a command to the display
// Param1: the command
// Param2: the values to change
void ERM19264_UC1609::send_command (uint8_t command, uint8_t value) 
{
  UC1609_CD_SetLow; 
  send_data(command | value);
  UC1609_CD_SetHigh;
}

// Desc: Resets LCD in a four wire setup called at start 
// and  should also be called in a controlled power down setting
void ERM19264_UC1609::LCDReset () 
{
  UC1609_RST_SetLow;
  delay(UC1609_RESET_DELAY); 
  UC1609_RST_SetHigh;
  delay(UC1609_RESET_DELAY2);
}

// Desc: turns in display
// Param1: bits 1  on , 0 off
void ERM19264_UC1609::LCDEnable (uint8_t bits) 
{
 UC1609_CS_SetLow;
  send_command(UC1609_DISPLAY_ON, bits);
 UC1609_CS_SetHigh;
}


// Desc: Scroll the displayed image up by SL rows. 
//The valid SL value is between 0 (for no
//scrolling) and (64). 
//Setting SL outside of this range causes undefined effect on the displayed
//image.
// Param1: bits 0-64 line number y-axis
void ERM19264_UC1609::LCDscroll (uint8_t bits) 
{
 UC1609_CS_SetLow;
  send_command(UC1609_SCROLL, bits);
 UC1609_CS_SetHigh;
}

// Desc: Rotates the display 
// Set LC[2:1] for COM (row) mirror (MY), SEG (column) mirror (MX).
// Param1: 4 possible values 000 010 100 110 (defined)
// If Mx is changed the buffer must BE updated
void ERM19264_UC1609::LCDrotate(uint8_t rotatevalue) 
{
 UC1609_CS_SetLow;
  switch (rotatevalue)
  {
      case 0: rotatevalue = 0; break;
      case 0x02: rotatevalue = UC1609_ROTATION_FLIP_ONE; break;
      case 0x04: rotatevalue = UC1609_ROTATION_NORMAL; break;
      case 0x06: rotatevalue = UC1609_ROTATION_FLIP_TWO; break;
      default: rotatevalue = UC1609_ROTATION_NORMAL; break;
  }
  send_command(UC1609_LCD_CONTROL, rotatevalue);
 UC1609_CS_SetHigh;
}

// Desc: invert the display
// Param1: bits, 1 invert , 0 normal
void ERM19264_UC1609::invertDisplay (uint8_t bits) 
{
 UC1609_CS_SetLow;
  send_command(UC1609_INVERSE_DISPLAY, bits);
 UC1609_CS_SetHigh;
}

// Desc: turns on all Pixels
// Param1: bits Set DC[1] to force all SEG drivers to output ON signals
// 1 all on ,  0 all off
void ERM19264_UC1609::LCD_allpixelsOn(uint8_t bits) 
{
 UC1609_CS_SetLow;
  send_command(UC1609_ALL_PIXEL_ON, bits);
 UC1609_CS_SetHigh;
}

// Desc: Fill the screen NOT the buffer with a datapattern 
// Param1: datapattern can be set to zero to clear screen (not buffer) range 0x00 to 0ff
// Param2: optional delay in microseconds can be set to zero normally.
void ERM19264_UC1609::LCDFillScreen(uint8_t dataPattern=0, uint8_t delay=0) 
{
 UC1609_CS_SetLow;
  uint16_t numofbytes = LCD_WIDTH * (LCD_HEIGHT /8); // width * height
  for (uint16_t i = 0; i < numofbytes; i++) 
  {
    send_data(dataPattern);
    delayMicroseconds(delay);
  }
UC1609_CS_SetHigh;
}

// Desc: Fill the chosen page(1-8)  with a datapattern 
// Param1: datapattern can be set to 0 to FF (not buffer)
void ERM19264_UC1609::LCDFillPage(uint8_t dataPattern=0) 
{
 UC1609_CS_SetLow;
  uint16_t numofbytes = ((LCD_WIDTH * (LCD_HEIGHT /8))/8); // (width * height/8)/8 = 192 bytes
  for (uint16_t i = 0; i < numofbytes; i++) 
  {
      send_data(dataPattern);
  }
 UC1609_CS_SetHigh;
}

//Desc: Draw a bitmap in PROGMEM to the screen
//Param1: x offset 0-192
//Param2: y offset 0-64
//Param3: width 0-192
//Param4 height 0-64
//Param5 the bitmap
void ERM19264_UC1609::LCDBitmap(int16_t x, int16_t y, uint8_t w, uint8_t h, const uint8_t* data) 
{
 UC1609_CS_SetLow;

  uint8_t tx, ty; 
  uint16_t offset = 0; 
  uint8_t column = (x < 0) ? 0 : x;
  uint8_t page = (y < 0) ? 0 : y >>3;

  for (ty = 0; ty < h; ty = ty + 8) 
  {
        if (y + ty < 0 || y + ty >= LCD_HEIGHT) {continue;}
        send_command(UC1609_SET_COLADD_LSB, (column & 0x0F)); 
        send_command(UC1609_SET_COLADD_MSB, (column & 0xF0) >> 4);
        send_command(UC1609_SET_PAGEADD, page++); 

        for (tx = 0; tx < w; tx++) 
        {
              if (x + tx < 0 || x + tx >= LCD_WIDTH) {continue;}
              offset = (w * (ty >> 3)) + tx; 
              send_data(pgm_read_byte(&data[offset]));
        }
  }
UC1609_CS_SetHigh;
}

// Desc: Checks if software SPI is on
// Returns: true 1 if hardware SPi on , false 0 for software spi
bool ERM19264_UC1609::isHardwareSPI() {
  return (_LCD_DIN == -1 && _LCD_SCLK == -1);
}

// Desc: used in software SPI mode to shift out data
// Param1: bit order LSB or MSB set to MSBFIRST for UC1609C
// Param2: the byte to go
// Other if using high freq MCU the delay define can be increased. 
void ERM19264_UC1609::CustomshiftOut(uint8_t bitOrder, uint8_t value)
{
    uint8_t i;

    for (i = 0; i < 8; i++)  {
        if (bitOrder == LSBFIRST)
            digitalWrite(_LCD_DIN, !!(value & (1 << i)));
        else    
            digitalWrite(_LCD_DIN, !!(value & (1 << (7 - i))));
            
        digitalWrite(_LCD_SCLK, HIGH);
        delayMicroseconds(UC1609_HIGHFREQ_DELAY);
        digitalWrite(_LCD_SCLK, LOW);
        delayMicroseconds(UC1609_HIGHFREQ_DELAY);
    }
}

//Desc: Send data byte with SPI to UC1609C
//Param1: the data byte
void ERM19264_UC1609::send_data(uint8_t byte)
{
  if (isHardwareSPI()) 
  {
     (void)SPI.transfer(byte); // Hardware SPI
  }else
  {
    CustomshiftOut(MSBFIRST, byte); //Software SPI
  }
} 

// Function to read "Get Status" registers 
// NOTE not working as of v 1.1.0 (returning 0x00)
uint16_t  ERM19264_UC1609::LCDreadStatus()
 {
	uint8_t resultbyte1 =0;
	uint8_t resultbyte2  = 0; 
	uint16_t ReturnResult = 0;  
	
	UC1609_CS_SetLow;
	send_command(UC1609_GET_STATUS,0);
	if (isHardwareSPI()) // Hardware SPI
	{
		resultbyte1 = SPI.transfer(0x00);
		resultbyte2 = SPI.transfer(0x00);
	}else 
	{
		resultbyte1 = shiftIn(_LCD_DIN, _LCD_SCLK, MSBFIRST);
		resultbyte2 = shiftIn(_LCD_DIN, _LCD_SCLK, MSBFIRST);
	}
	
	UC1609_CS_SetHigh;
	
	 resultbyte1 = resultbyte1<< 8;
	 ReturnResult = resultbyte1| resultbyte2;
	 return ReturnResult;
 }
 
// ******************************************************
// Functions below not & needed for no_buffer mode 
#ifndef NO_BUFFER

//Desc: updates the buffer i.e. writes it to the screen
void ERM19264_UC1609::LCDupdate() 
{

#ifdef MULTI_BUFFER
      LCDBuffer( this->ActiveBuffer->xoffset, this->ActiveBuffer->yoffset, this->ActiveBuffer->width, this->ActiveBuffer->height, (uint8_t*) this->ActiveBuffer->screenbitmap); 
      return;
#endif

#ifdef SINGLE_BUFFER
  uint8_t x = 0; uint8_t y = 0; uint8_t w = this->bufferWidth; uint8_t h = this->bufferHeight;
  LCDBuffer( x,  y,  w,  h, (uint8_t*) this->buffer);
#endif
}

//Desc: clears the buffer i.e. does NOT write to the screen
void ERM19264_UC1609::LCDclearBuffer()
{
#ifdef MULTI_BUFFER
   memset( this->ActiveBuffer->screenbitmap, 0x00, (this->ActiveBuffer->width * (this->ActiveBuffer->height/ 8))  ); 
   return;
#endif

#ifdef SINGLE_BUFFER
  memset( this->buffer, 0x00, (this->bufferWidth * (this->bufferHeight /8))  ); 
#endif
}

//Desc: Draw a bitmap to the screen
//Param1: x offset 0-192
//Param2: y offset 0-64
//Param3: width 0-192
//Param4 height 0-64
//Param5 the bitmap
void ERM19264_UC1609::LCDBuffer(int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t* data) 
{
 UC1609_CS_SetLow;

  uint8_t tx, ty; 
  uint16_t offset = 0; 
  uint8_t column = (x < 0) ? 0 : x;
  uint8_t page = (y < 0) ? 0 : y/8;

  for (ty = 0; ty < h; ty = ty + 8) 
  {
    if (y + ty < 0 || y + ty >= LCD_HEIGHT) {continue;}
    
    send_command(UC1609_SET_COLADD_LSB, (column & 0x0F)); 
    send_command(UC1609_SET_COLADD_MSB, (column & 0XF0) >> 4); 
    send_command(UC1609_SET_PAGEADD, page++); 
 
    for (tx = 0; tx < w; tx++) 
    {
          if (x + tx < 0 || x + tx >= LCD_WIDTH) {continue;}
          offset = (w * (ty /8)) + tx; 
          send_data(data[offset++]);
    }
  }
  
UC1609_CS_SetHigh;
}

// Desc: Draws a Pixel to the screen overides the custom graphics library
// Passed x and y co-ords and colour of pixel.
void ERM19264_UC1609::drawPixel(int16_t x, int16_t y, uint16_t colour) 
{
    
#ifdef MULTI_BUFFER
  if ((x < 0) || (x >= this->ActiveBuffer->width) || (y < 0) || (y >= this->ActiveBuffer->height)) {
    return;
  }
      uint16_t offset = (this->ActiveBuffer->width * (y/8)) + x; 
      switch (colour)
      {
        case FOREGROUND: this->ActiveBuffer->screenbitmap[offset] |= (1 << (y & 7)); break;
        case BACKGROUND: this->ActiveBuffer->screenbitmap[offset] &= ~(1 << (y & 7)); break;
        case INVERSE: this->ActiveBuffer->screenbitmap[offset] ^= (1 << (y & 7)); break;
      }
    return;
#endif
    
#ifdef SINGLE_BUFFER
  if ((x < 0) || (x >= this->bufferWidth) || (y < 0) || (y >= this->bufferHeight)) {
    return;
  }
      uint16_t tc = (bufferWidth * (y /8)) + x; 
      switch (colour)
      {
        case FOREGROUND:  this->buffer[tc] |= (1 << (y & 7)); break;
        case BACKGROUND:  this->buffer[tc] &= ~(1 << (y & 7)); break;
        case INVERSE: this->buffer[tc] ^= (1 << (y & 7)); break;
      }
#endif
}

// ***************************************************
#else 
//  these functions are not needed by buffers modes
// and only used in "no buffer" mode
// ***********************************************

// Desc: goes to XY position
// Param1 : coloumn 0-192
// Param2  : page 0-7
void ERM19264_UC1609::LCDNoBufferGotoXY(uint8_t column , uint8_t page)
{
        UC1609_CS_SetLow;
        send_command(UC1609_SET_COLADD_LSB, (column & 0x0F)); 
        send_command(UC1609_SET_COLADD_MSB, (column & 0xF0) >> 4);
        send_command(UC1609_SET_PAGEADD, page++); 
        UC1609_CS_SetHigh;
}

// Desc: draws passed character.
// Param1: character 'A' or number in  the ASCII table 1-127(default)
void ERM19264_UC1609::LCDNoBufferChar(char character)
{
   UC1609_CS_SetLow;
   UC1609_FONTPADDING;
    for (uint8_t  column = 0 ; column <  UC1609_FONTWIDTH ; column++)
    {
        send_data((pgm_read_byte(custom_font + (character*UC1609_FONTWIDTH) + column)));
    }
    UC1609_FONTPADDING;
    UC1609_CS_SetHigh;
}

// Desc: draws passed  character array
// Param1: pointer to start of character array
void ERM19264_UC1609::LCDNoBufferString(const char *characters)
{
    while (*characters)
    LCDNoBufferChar(*characters++);
}
#endif
//***********************************************
