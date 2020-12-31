#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>       // for fs function like alloc_chrdev_region / operation file
#include <linux/types.h>
#include <linux/device.h>   // for device_create and class_create
#include <linux/uaccess.h>  // for copy to/from user function
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/of.h>       // access device tree file
#include <linux/delay.h>
#include <linux/slab.h>     // kmalloc, kcallloc, ....
#include <linux/mutex.h>
#include <linux/of_gpio.h>
#include <linux/kthread.h>	//create thread
#include <linux/kdev_t.h>	//define MAJOR, MINOR, MKDEV, ...
#include <linux/sched.h>	//task_struct
#include <linux/cdev.h>
#include <linux/ioctl.h>

#define DRIVER_NAME "get-gpio-led"
#define FIRST_MINOR 0
#define DEFAULT_RESET_TIME 25
#define DEFAULT_BOOT_TIME 10
#define WR_VALUE _IOW('a','a',int32_t*)
#define RD_VALUE _IOR('a','b',int32_t*)
int32_t value = 0;
int32_t value1 = 0;

#define BUFFER_SIZE 256


#define PDEBUG(fmt,args...) printk(KERN_DEBUG"%s:"fmt,DRIVER_NAME, ##args)
#define PERR(fmt,args...) printk(KERN_ERR"%s:"fmt,DRIVER_NAME,##args)
#define PINFO(fmt,args...) printk(KERN_INFO"%s:"fmt,DRIVER_NAME, ##args)

typedef struct gpio_data{
    unsigned gpio;
    bool active_low;
} gpio_data_t;

typedef struct dev_private_data{
	struct mutex lock;
    struct device *dev;
    struct cdev cdev;
    const char *name;
    gpio_data_t led_gpio;
    int delay_time;
	struct task_struct *thread_blink_led;
} dev_private_data_t;

typedef struct platform_private_data{
    struct class * dev_class;
    int num_reset;
    dev_private_data_t devices [];
} platform_private_data_t;

dev_t dev_num ;
int dev_num_major = 0;
static int blink_led(void* param);

int atoi_t(const char* str){
    int num = 0;
    int i = 0;
    bool isNegetive = false;
    if(str[i] == '-'){
        isNegetive = true;
        i++;
    }
    while (str[i] && (str[i] >= '0' && str[i] <= '9')){
        num = num * 10 + (str[i] - '0');
        i++;
    }
    if(isNegetive) num = -1 * num;
    return num;
}

static inline int sizeof_platform_data(int num_reset)
{
	return sizeof(platform_private_data_t) +
		(sizeof(dev_private_data_t) * num_reset);
}

static int blink_led(void* param){
	int i;
	dev_private_data_t *data = (dev_private_data_t *)param;
	PINFO ("Blink time while true!\n");
	while(!kthread_should_stop())
	{	gpio_set_value(data->led_gpio.gpio, 1 ^ data->led_gpio.active_low);
		if(data->delay_time < 1000){
			msleep(data->delay_time);
		}
		else{
			for(i = 0; i < (data->delay_time/1000); i++){
				msleep(1000);
			}
		}
		gpio_set_value(data->led_gpio.gpio, 0 ^ data->led_gpio.active_low);
		if(data->delay_time < 1000){
			msleep(data->delay_time);
		}
		else{
			for(i = 0; i < (data->delay_time/1000); i++){
				msleep(1000);
			}
		}
	}
	return 0;
}
/*YNGUYEN*/
/***** Define device attribute *****/
static ssize_t blink_time_store(struct device *dev, struct device_attribute *attr, const char *buff, size_t len)
{
    dev_private_data_t *data = dev_get_drvdata(dev);
    if (!data){
        PERR ("Can't get device private data\n");
	}
	data->delay_time = atoi_t(buff);
	if(data->delay_time < 0 || data->delay_time > 10000000){
		PINFO ("Delay-time input out of range [0;10000000], Retype please!\n");
		return -1;
	}
	else{
			PINFO ("Blink time start!\n");
	}
    return len;
}
/*YNGUYEN*/
static DEVICE_ATTR_WO(blink_time);
#ifdef CONFIG_LEDS_TRIGGERS
static DEVICE_ATTR(trigger, 0644, NULL,blink_time_store);
static struct attribute *led_trigger_attrs[] = {
	&dev_attr_trigger.attr,
	NULL,
};
static struct attribute_group led_trigger_group = {
	.attrs = led_trigger_attrs,
};
#endif

static struct attribute *led_class_attrs[] = {
	&dev_attr_blink_time.attr,
	NULL,
};

static struct attribute_group led_group = {
	.attrs = led_class_attrs,
};

static const struct attribute_group *led_groups[] = {
	&led_group,
#ifdef CONFIG_LEDS_TRIGGERS
	&led_trigger_group,
#endif
	NULL,
};
/*YNGUYEN*/

/*******************************************************/
static int get_gpio_led_open(struct inode *inode,struct file *filp)
{
	dev_private_data_t *priv = container_of(inode->i_cdev, dev_private_data_t , cdev);
	filp->private_data = priv;
	PINFO("In char driver open() function\n");
	return 0;
}

static int get_gpio_led_release(struct inode *inode,struct file *filp)
{
	dev_private_data_t *priv;
	priv=filp->private_data;
	PINFO("In char driver release() function\n");
	return 0;
}

static ssize_t get_gpio_led_read(struct file *filp, char __user *ubuff,size_t count,loff_t *offp)
{
	int err_count=0;
	char str[BUFFER_SIZE] = {0};
	dev_private_data_t *priv;
	priv = filp->private_data;
	sprintf(str,"%d",priv->delay_time);
	err_count = copy_to_user(ubuff, str, strlen(str));
	if(err_count == 0)
	{
		PINFO( "Sent chars to the user\n");
		return 0;
	}
	else{
		PERR( "Failed to send %d chars to the user\n", err_count);
		return -EFAULT;
	}
} 

static ssize_t get_gpio_led_write(struct file *filp,
	const char __user *ubuff, size_t count, loff_t *offp)
{
	char msg[BUFFER_SIZE] = {0};
	dev_private_data_t *priv;
	priv = filp->private_data;
  
	if(copy_from_user(msg, ubuff, count)) {
		PERR("ERROR: Not all the bytes have been copied from user\n");
		return -EFAULT;
	}
	sscanf(msg, "%d", &value1);
	
	priv->delay_time = value1;
	PINFO ("Delay-time which user typed: %d\n", priv->delay_time);
	return count;
}

static long get_gpio_led_ioctl(struct file *filp, unsigned int cmd , unsigned long arg)
{
	int n = -1;
	dev_private_data_t *priv;
	priv = filp->private_data;
	PINFO("In char driver ioctl() function\n");
	switch(cmd){
		case WR_VALUE:
		{
            if(copy_from_user(&value ,(int32_t*) arg, sizeof(value))){
            	PDEBUG("in ioctl: Cannot copy from user");
            	return -EFAULT;
            }
            priv->delay_time = value;
            PINFO ("Delay-time which user typed: %d\n", value);
            break;
		}
        case RD_VALUE:
        {
			n = copy_to_user((int32_t*) arg, &value, sizeof(value));
            if(n<0)
			{
				PDEBUG("in ioctl: Cannot read Delay-time from device");
            	return -EFAULT;
			}
            break;
        }
        default:
        {}
	}
	return 0;
}
static const struct file_operations get_gpio_led_fops= {
	.owner				= THIS_MODULE,
	.open				= get_gpio_led_open,
	.release			= get_gpio_led_release,
	.read				= get_gpio_led_read,
	.write				= get_gpio_led_write,
	.unlocked_ioctl		= get_gpio_led_ioctl,
};
/*******************************************************/

/*****Module INIT + EXIT****/
static int driver_probe (struct platform_device *pdev)
{
    int res, intRequest;
    u32 temp;
    struct device_node *np = pdev->dev.of_node;
    dev_private_data_t *device;
	platform_private_data_t *data;
	PINFO ("Driver module INIT \n");
	PINFO ("Node name: %s\n",pdev->dev.of_node->name );
	res = alloc_chrdev_region(&dev_num, FIRST_MINOR, 1, DRIVER_NAME);// Register device
	if (res<0){
	PERR("Can't register device, error code: %d \n", res);
	return -1;
	}
	PINFO ("Register device pass \n");
	dev_num_major = MAJOR(dev_num);
	data = (platform_private_data_t*)kcalloc(1, sizeof_platform_data(1), GFP_KERNEL);// Create private data
	data->dev_class = class_create(THIS_MODULE, DRIVER_NAME);// Create class
	if (IS_ERR(data->dev_class)){
		PERR("Class create fail\n");
		goto error_class;
	}
	data->dev_class->dev_groups = led_groups;
	/*YNGUYEN*/
	device = &data->devices[0];
	/****************/
    mutex_init(&device->lock);
    cdev_init(&device->cdev, &get_gpio_led_fops);
    if(cdev_add(&device->cdev, dev_num, 1) < 0)
	{
		PERR ("Can't add device");
	}
	/****************/
	device->name = of_get_property(np, "get-gpio-led", NULL) ? : np->name;
	device->led_gpio.active_low = of_property_read_bool(np, "led-active-low");
	PINFO ("Value of led-active-low from device tree: %d\n", device->led_gpio.active_low);
	res = of_property_read_u32(np, "delay-time", &temp);
	if (!res){
		device->delay_time = temp;
		PINFO ("Delay time from device tree (pass): %d\n", temp);
	}
	else{
		device->delay_time = DEFAULT_RESET_TIME;
		PINFO ("Delay time from DEFAULT_RESET_TIME: %d\n", DEFAULT_RESET_TIME);
	}
	PINFO ("Name pdev->name: %s \n",pdev->name);
	device->dev = device_create(data->dev_class, &pdev->dev, dev_num, device, "%s", device->name);
	//device->dev = device_create_with_groups(data->dev_class, &pdev->dev, 0, device, pdev->dev.groups, "%s", device->name);
	device->led_gpio.gpio = of_get_named_gpio(np, "gpio-pin", 0);
	if (!gpio_is_valid(device->led_gpio.gpio)){
		PINFO ("GPIO number is invalid \n");
		goto error_gpio_init;
	}
	else{
		intRequest = devm_gpio_request_one(device->dev, device->led_gpio.gpio, GPIOF_OUT_INIT_LOW, "gpio-pin"); //Request gpio
		PINFO ("Value off devm_gpio_request_one: %d\n", intRequest);
		if(intRequest<0){
			PERR ("Can't get request gpio-led device");
		}
		else{
			PINFO ("Turn on GPIO 21\n");
		}
	}
	device->thread_blink_led = (struct task_struct*)kcalloc(1, sizeof(struct task_struct), GFP_KERNEL);
	device->thread_blink_led = kthread_run(blink_led, device, "Blink led start!");
	// device->thread_blink_led = kthread_create(blink_led, NULL, "Create Blink led thread!");
	if(device->thread_blink_led){
			PINFO ("Kthread Created Successfully...\n");
        }
	else{
			PERR ("Can't create kthread\n");
            goto r_device;
        }
	platform_set_drvdata(pdev, data);
	return 0;
	error_gpio_init:
		gpio_free(device->led_gpio.gpio);
	error_class:
    	return -1;
	r_device:
		return -1;
}
	/*YNGUYEN*/


static int driver_remove(struct platform_device *pdev)
{
    int i;
    platform_private_data_t *data = platform_get_drvdata(pdev);
    if (!data){
        PERR ("Can't get device private data");
	}
    PINFO("Driver module remove from kernel\n");
    kthread_stop(data->devices->thread_blink_led);
    for(i = 0 ; i < data->num_reset; ++i){
        gpio_free(data->devices[i].led_gpio.gpio);
        dev_num = MKDEV(dev_num_major, FIRST_MINOR + i);
        device_destroy(data->dev_class, dev_num);
        cdev_del(&data->devices[i].cdev);
    }
    class_destroy(data->dev_class);
    kfree(data);
    platform_set_drvdata(pdev, NULL);
    return 0;
}

static const struct of_device_id reset_dst[] = {
    { .compatible = "get-gpio-led", },
    {}
};

MODULE_DEVICE_TABLE(of, reset_dst);

static struct platform_driver gpio_isp = {
    .driver = {
        .name = DRIVER_NAME,
        .owner = THIS_MODULE,
        .of_match_table = of_match_ptr(reset_dst),
    },
    .probe = driver_probe,
    .remove = driver_remove,
};

module_platform_driver(gpio_isp);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("YNGUYEN");






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
Run: echo <delay_time you want> > /sys/class/get-gpio-led/led-gpio/mode
Note: <delay_time you want> is an integer in the range [0;100], unit: seconds
*/
