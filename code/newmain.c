#define _XTAL_FREQ 8000000
#include <xc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// CONFIG
#pragma config FOSC = HS
#pragma config WDTE = OFF
#pragma config PWRTE = OFF
#pragma config BOREN = OFF
#pragma config LVP = OFF
#pragma config CPD = OFF
#pragma config WRT = OFF
#pragma config CP = OFF

// ===== STACK CHAR =====
typedef struct {
    char arr[20];
    int top;
} Stack;

void stack_init(Stack* s) { s->top = -1; }
int stack_empty(Stack* s) { return s->top == -1; }
void stack_push(Stack* s, char c) { if(s->top < 19) s->arr[++s->top] = c; }
char stack_pop(Stack* s) { return stack_empty(s) ? 0 : s->arr[s->top--]; }
char stack_peek(Stack* s) { return s->arr[s->top]; }

// ===== STACK INT =====
typedef struct {
    int arr[20];
    int top;
} IntStack;

void int_stack_init(IntStack* s) { s->top = -1; }
int int_stack_empty(IntStack* s) { return s->top == -1; }
void int_stack_push(IntStack* s, int val) { if(s->top < 19) s->arr[++s->top] = val; }
int int_stack_pop(IntStack* s) { return int_stack_empty(s) ? 0 : s->arr[s->top--]; }

// ===== TOÁN T? =====
int precedence(char c) {
    if (c == '+' || c == '-') return 1;
    if (c == '*' || c == '/') return 2;
    return 0;
}
int infix_compare(char c1, char c2) {
    return precedence(c1) >= precedence(c2);
}

// ===== INFIX to POSTFIX =====
void infix_to_postfix(const char* infix, char* postfix) {
    Stack s;
    stack_init(&s);
    int i = 0, j = 0;

    while (infix[i]) {
        if (infix[i] >= '0' && infix[i] <= '9') {
            while (infix[i] >= '0' && infix[i] <= '9') {
                postfix[j++] = infix[i++];
            }
            postfix[j++] = ' ';
        } else if (infix[i] == '+' || infix[i] == '-' || infix[i] == '*' || infix[i] == '/') {
            while (!stack_empty(&s) && infix_compare(stack_peek(&s), infix[i])) {
                postfix[j++] = stack_pop(&s);
                postfix[j++] = ' ';
            }
            stack_push(&s, infix[i]);
            i++;
        } else {
            i++; // b? qua ký t? không h?p l?
        }
    }

    while (!stack_empty(&s)) {
        postfix[j++] = stack_pop(&s);
        postfix[j++] = ' ';
    }

    postfix[j] = '\0';
}

// ===== TÍNH POSTFIX =====
int postfix_cal(const char* postfix) {
    IntStack s;
    int_stack_init(&s);
    int i = 0;
    char token[10];
    int token_index = 0;

    while (postfix[i]) {
        if (postfix[i] >= '0' && postfix[i] <= '9') {
            token_index = 0;
            while (postfix[i] >= '0' && postfix[i] <= '9') {
                token[token_index++] = postfix[i++];
            }
            token[token_index] = '\0';
            int_stack_push(&s, atoi(token));
        } else if (postfix[i] == '+' || postfix[i] == '-' || postfix[i] == '*' || postfix[i] == '/') {
            int b = int_stack_pop(&s);
            int a = int_stack_pop(&s);
            int result = 0;
            switch(postfix[i]) {
                case '+': result = a + b; break;
                case '-': result = a - b; break;
                case '*': result = a * b; break;
                case '/': result = b != 0 ? a / b : 0; break;
            }
            int_stack_push(&s, result);
        }
        i++;
    }

    return int_stack_pop(&s);
}

// ===== LCD 4 BIT =====
void lcd_nibble(unsigned char nibble) {
    PORTBbits.RB4 = (nibble >> 0) & 1;
    PORTBbits.RB5 = (nibble >> 1) & 1;
    PORTBbits.RB6 = (nibble >> 2) & 1;
    PORTBbits.RB7 = (nibble >> 3) & 1;
}

void lcd_pulse() {
    PORTBbits.RB2 = 1;
    __delay_us(1);
    PORTBbits.RB2 = 0;
    __delay_us(100);
}

void lcd_byte(unsigned char byte, unsigned char mode) {
    PORTBbits.RB0 = mode;
    lcd_nibble(byte >> 4);
    lcd_pulse();
    lcd_nibble(byte & 0x0F);
    lcd_pulse();
}

void lcd_init() {
    PORTB = 0x00;
    lcd_nibble(0x03); lcd_pulse(); __delay_ms(15);
    lcd_nibble(0x03); lcd_pulse(); __delay_ms(5);
    lcd_nibble(0x03); lcd_pulse();
    lcd_nibble(0x02); lcd_pulse();
    lcd_byte(0x28, 0); // 4-bit, 2 dòng
    lcd_byte(0x0C, 0); // Hi?n th? ON
    lcd_byte(0x01, 0); // Clear
    __delay_ms(2);
    lcd_byte(0x06, 0); // T?ng ??a ch?
}

void lcd_write_char(char c) { lcd_byte(c, 1); }
void lcd_write_string(const char* s) { while(*s) lcd_write_char(*s++); }

// ===== KEYPAD 4x4 =====
char keypad[4][4] = {
    {'7','8','9','/'},
    {'4','5','6','*'},
    {'1','2','3','-'},
    {'C','0','=','+'}
};

// ===== MAIN =====
void main() {
    // LCD setup
    TRISB = 0x00;
    lcd_init();

    // Keypad setup
    TRISD = 0x0F; // RD0-3 input, RD4-7 output
    PORTD = 0xF0;

    char mem[32] = {0};
    int index = 0;

    while(1) {
        for (int row = 0; row < 4; row++) {
            PORTD = 0xF0;
            PORTD &= ~(1 << (row + 4));
            __delay_ms(5);

            for (int col = 0; col < 4; col++) {
                if (!(PORTD & (1 << col))) {
                    char key = keypad[row][col];
                    lcd_write_char(key);
                    mem[index++] = key;

                    if (key == '=') {
                        mem[index - 1] = '\0';
                        char postfix[32];
                        infix_to_postfix(mem, postfix);
                        int result = postfix_cal(postfix);
                        lcd_byte(0x01, 0); __delay_ms(2);
                        char result_str[10];
                        sprintf(result_str, "%d", result);
                        lcd_write_string(result_str);
                        index = 0;
                        memset(mem, 0, sizeof(mem));
                    }
                    if (key == 'C') {
                        lcd_byte(0x01, 0);
                        index = 0;
                        memset(mem, 0, sizeof(mem));
                    }
                    
                    while (!(PORTD & (1 << col)));
                    __delay_ms(200);
                }
            }
        }
    }
}
