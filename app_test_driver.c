#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <fcntl.h>
 
//#define WR_VALUE _IOW('a','a',int32_t*)
//#define RD_VALUE _IOR('a','b',int32_t*)
static char receive[256];
void reverse(char s[])
 {
     int i, j;
     char c;

     for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
         c = s[i];
         s[i] = s[j];
         s[j] = c;
     }
}  
void itoa(int n, char s[])
 {
     int i, sign;

     if ((sign = n) < 0)  /* record sign */
         n = -n;          /* make n positive */
     i = 0;
     do {       /* generate digits in reverse order */
         s[i++] = n % 10 + '0';   /* get next digit */
     } while ((n /= 10) > 0);     /* delete it */
     if (sign < 0)
         s[i++] = '-';
     s[i] = '\0';
     reverse(s);
}  
int main()
{
        int fd, number;
        char delaytimeSend[256];
        printf("*********************************\n");
        printf("*******APP TESTS DRIVER*******\n");
 
        printf("\nOpening Driver\n");
        fd = open("/dev/gpio_led", O_RDWR);
        if(fd < 0) {
                printf("Cannot open device file...\n");
                return 0;
        }
 
        printf("Enter the Value to send\n");
        scanf("%d",&number);
        //scanf("%[^\n]%*c", delaytimeSend);
        itoa(number, delaytimeSend);
        printf("Writing Delay-time to Driver\n");
        //ioctl(fd, WR_VALUE, (int32_t*) &number);
        write(fd, delaytimeSend, strlen(delaytimeSend));

        printf("Reading Delay-time from Driver\n");
        //ioctl(fd, RD_VALUE, (int32_t*) &value);
        read(fd,receive,256);
        printf("Delay-time is %s\n", receive);
 
        printf("Closing Driver\n");
        close(fd);
}
