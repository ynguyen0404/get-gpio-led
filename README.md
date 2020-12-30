# get-gpio-led
#device tree
/*msm8953-mtp.dtsi*/
/*
*	gpio_led {
*			compatible = "get-gpio-led";
*			status = "okay";
*			gpio-pin = <&tlmm 21 0x1>;
*			delay-time = <500>;
*			led-active-low;
*		};
*/
/*msm8953-mtp.dtsi*/

/*msm8953-pinctrl.dtsi*/
/*
*			led_gpio: led_gpio {
*				//ENABLE
*				mux {
*					pins = "gpio21";
*					function = "gpio";
*				};
*
*				config {
*					pins = "gpio21";
*					bias-disable;
*					drive-strength = <2>;
*				};
*			};
*/
/*msm8953-pinctrl.dtsi*/

/*
Run: echo <delay_time you want> > /sys/class/get-gpio-led/led-gpio/blink-time
Note: <delay_time you want> is an integer, unit: mlseconds
*/
/*
Run IOCTL: /vendor/bin/gpio_led => type <Delay-time>
Run Read/Write: /vendor/bin/gpio_led => type <Delay-time>
*/
