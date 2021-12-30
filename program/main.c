//Work inspired from https://github.com/pololu/pololu-led-strip-avr, especially for the led_strip_write function. 
//Works for CPU frequence of 8 MHz (ATtiny85)

#define F_CPU 8000000
#define LED_STRIP_PORT PORTB
#define LED_STRIP_DDR  DDRB
#define LED_STRIP_PIN  0
#define LED_COUNT 18


#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <math.h>


typedef struct rgb_color
{
  uint8_t red, green, blue;
} rgb_color;

void __attribute__((noinline)) led_strip_write(rgb_color * colors, uint16_t count)
{
  // Set the pin to be an output driving low.
  LED_STRIP_PORT &= ~(1<<LED_STRIP_PIN);
  LED_STRIP_DDR |= (1<<LED_STRIP_PIN);

  cli();   // Disable interrupts temporarily because we don't want our pulse timing to be messed up.
  while (count--)
  {
    // Send a color to the LED strip.
    // The assembly below also increments the 'colors' pointer,
    // it will be pointing to the next color at the end of this loop.
    asm volatile (
        "ld __tmp_reg__, %a0+\n"
        "ld __tmp_reg__, %a0\n"
        "rcall send_led_strip_byte%=\n"  // Send red component.
        "ld __tmp_reg__, -%a0\n"
        "rcall send_led_strip_byte%=\n"  // Send green component.
        "ld __tmp_reg__, %a0+\n"
        "ld __tmp_reg__, %a0+\n"
        "ld __tmp_reg__, %a0+\n"
        "rcall send_led_strip_byte%=\n"  // Send blue component.
        "rjmp led_strip_asm_end%=\n"     // Jump past the assembly subroutines.

        // send_led_strip_byte subroutine:  Sends a byte to the LED strip.
        "send_led_strip_byte%=:\n"
        "rcall send_led_strip_bit%=\n"  // Send most-significant bit (bit 7).
        "rcall send_led_strip_bit%=\n"
        "rcall send_led_strip_bit%=\n"
        "rcall send_led_strip_bit%=\n"
        "rcall send_led_strip_bit%=\n"
        "rcall send_led_strip_bit%=\n"
        "rcall send_led_strip_bit%=\n"
        "rcall send_led_strip_bit%=\n"  // Send least-significant bit (bit 0).
        "ret\n"

        // send_led_strip_bit subroutine:  Sends single bit to the LED strip by driving the data line
        // high for some time.  The amount of time the line is high depends on whether the bit is 0 or 1,
        // but this function always takes the same time (2 us).
        "send_led_strip_bit%=:\n"

        "rol __tmp_reg__\n"                      // Rotate left through carry.

        "sbi %2, %3\n"                           // Drive the line high.

        "brcs .+2\n" "cbi %2, %3\n"              // If the bit to send is 0, drive the line low now.

        "nop\n" "nop\n"
        "brcc .+2\n" "cbi %2, %3\n"              // If the bit to send is 1, drive the line low now.

        "ret\n"
        "led_strip_asm_end%=: "
        : "=b" (colors)
        : "0" (colors),         // %a0 points to the next color to display
          "I" (_SFR_IO_ADDR(LED_STRIP_PORT)),   // %2 is the port register (e.g. PORTC)
          "I" (LED_STRIP_PIN)     // %3 is the pin number (0-8)
    );
  }
  sei();          // Re-enable interrupts now that we are done.
  _delay_us(80);  // Send the reset signal.
}


int min(int a, int b)
{
        return (a > b) ? b : a;
}

int max(int a, int b)
{
        return (a < b) ? b : a;
}

int clamp(int y)
{
   return (y<0)?0:((y>255)?255:y);
}

int thirdfunc(float x)
{
        return clamp(((cos(x)*cos(x)*(-1)-2*sin(x))/2)*192 + 128);
}

rgb_color colors[LED_COUNT];
rgb_color black[LED_COUNT];
uint8_t button = 0;

ISR(INT0_vect){
  if( (PINB & (1 << PINB2)) == 0){
          button = 1;
          _delay_ms(50);
  }
}

void blink(uint8_t nb)
{
  uint8_t c = 0;
  while(c != nb && button == 0)
  {
    led_strip_write(colors, LED_COUNT);
    _delay_ms(400);
    GIMSK |= (1<<INT0);
    sei();
    if(button == 0)
    { 
    led_strip_write(black, LED_COUNT);
    _delay_ms(400);
    GIMSK |= (1<<INT0);
    sei();
    } 
    c += 1;
  }
}

void blinklong(uint8_t nb)
{
  uint8_t c = 0;
  while(c != nb && button == 0)
  {
    led_strip_write(colors, LED_COUNT);
    _delay_ms(800);
    GIMSK |= (1<<INT0);
    sei();
    if(button == 0)
    { 
    led_strip_write(black, LED_COUNT);
    _delay_ms(800);
    GIMSK |= (1<<INT0);
    sei();
    } 
    c += 1;
  }
}

int main()
{
  PORTB |=(1<<PB2);
  uint16_t time = 0;
  uint8_t b = 0;
  float a = 0;
  while(1)
  { 
    while(button == 0) //ANIM 1
    {
      while (time < 50000 && button == 0)
      {
        uint16_t red = thirdfunc(1.3*a);
        uint16_t grn = thirdfunc(0.8*a+2.09);
        uint16_t blu = thirdfunc(1.2*a+4.18);
        for (uint16_t i = 0; i < LED_COUNT; i++)
        {
          uint8_t x = (time >> 2) - 8 * i;
          uint8_t y = (time >> 1) - 8 * i;
          colors[i] = (rgb_color){x*red/255,y*grn/255,blu};
        }
        led_strip_write(colors, LED_COUNT);
        _delay_ms(20); //Time before refreshing colors on strip
        time += 20;
        a += 0.005;
        if (a>12.48)
        {
                a = 0;
        }
        GIMSK |= (1<<INT0);
        sei(); 
      }
      time = 0;
      while (time<100000 && button == 0)
      {
        uint16_t red = thirdfunc(0.6*a);
        uint16_t grn = thirdfunc(1.3*a+2.09);
        uint16_t blu = thirdfunc(0.7*a+4.18);
        for (uint16_t i = 0; i < LED_COUNT; i++)
        {
          colors[i] = (rgb_color){blu,grn,red};
        }
        led_strip_write(colors, LED_COUNT);
        _delay_ms(30); //Time before refreshing colors on strip
        time += 30;
        a += 0.01;
        if (a>12.48)
        {
                a = 0;
        }
        GIMSK |= (1<<INT0);
        sei(); 
      }
      time=0;
    }
    button = 0;
    time = 0;
    a = 0;
    
    while(button == 0)
    {
      uint16_t red = thirdfunc(1.3*a);
      uint16_t grn = thirdfunc(0.8*a+2.09);
      uint16_t blu = thirdfunc(1.2*a+4.18);
      for (uint16_t i = 0; i < LED_COUNT; i++)
      {
        colors[i] = (rgb_color){red,grn,blu};
      }
      led_strip_write(colors, LED_COUNT);
      _delay_ms(20); //Time before refreshing colors on strip
      a += 0.01;
      if (a>12.48)
      {
              a = 0;
      }
    }
    button = 0;

    while(button == 0) //ANIM 3
    {
      while (b == 0 && button == 0)
      {
        for (uint16_t i = 0; i < LED_COUNT; i++)
        {
          uint8_t x = (time >> 2) - 8 * i;
          colors[0+i] = (rgb_color){ x, x, 255-x };
        }
    
        led_strip_write(colors, LED_COUNT);
    
        _delay_ms(20);
        time += 20;
        if(time > 1600)
        {
            b = 1;
            time = 0;
        }
        GIMSK |= (1<<INT0);
        sei();
      }
      b = 0;
      while (b == 0 && button == 0)
      {
        for (uint16_t i = 0; i < LED_COUNT; i++)
        {
          uint8_t x = (time >> 2) - 8 * i;
          colors[17-i] = (rgb_color){ x, 255 - x, x/i };
        }
    
        led_strip_write(colors, LED_COUNT);
    
        _delay_ms(20);
        time += 20;
        if(time > 3000)
        {
            b = 1;
        }
        GIMSK |= (1<<INT0);
        sei(); 
      }
      b = 0;
      time=0;
      if(button == 0)
      {
        _delay_ms(1000);
      }
    }
    button = 0;

    //ANIM 4 - SOS
    for (uint16_t i = 0; i < LED_COUNT; i++)
    {
      black[i] = (rgb_color){ 0,0,0 };
    }
  
    while(button == 0)
    {
      blink(3);
      blinklong(3);
      blink(3);
      _delay_ms(1000);
    }
    button = 0;
  }
}
