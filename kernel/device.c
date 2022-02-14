/*
 *	ProPuT -- Device manipulation
 *
 *	(c) 2022 Maria Matejka <mq@jmq.cz>
 *
 *	Can be freely distributed and used under the terms of the GNU GPL.
 */

#include "module.h"

#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/gpio/consumer.h>
#include <linux/of.h>
#include <linux/of_platform.h>

static struct proput_dev_data {
  struct device *dev;
  int num_gpios;
  struct gpio_desc **gpios;
} pdd;

OP_DEF(cmd_devlist)
{
  struct proput_response_header rh;

  if (sz != sizeof(*hdr))
    return -EINVAL;

  /* No devices yet to be listed */
  rh.response = PROPUT_R_DEVLIST;
  rh.len = sizeof(rh);

  return ufp_response(ctx, &rh);
}

static int proput_probe(struct platform_device *pdev)
{
  int i;

  pdd.dev = &pdev->dev;
  pdd.num_gpios = gpiod_count(pdd.dev, NULL);
  if (pdd.num_gpios < 0)
  {
    dev_err(pdd.dev, "GPIO pins garbled");
    return -ENODEV;
  }

  pdd.gpios = devm_kcalloc(pdd.dev, pdd.num_gpios, sizeof(struct gpio_desc *), GFP_KERNEL);
  if (!pdd.gpios)
    return -ENOMEM;

  for (i = 0; i < pdd.num_gpios; i++) {
    pdd.gpios[i] = devm_gpiod_get_index(pdd.dev, NULL, i, GPIOD_ASIS);
    if (IS_ERR(pdd.gpios[i]))
      return PTR_ERR(pdd.gpios[i]);
  }

//  platform_set_drvdata(pdev, &pdd);
  dev_info(pdd.dev, "proput initialized with %d gpios\n", pdd.num_gpios);

  return 0;
}

static void proput_shutdown(struct platform_device *pdev)
{
  /* Nothing to do */
}

#ifdef CONFIG_PM_SLEEP
static int proput_suspend(struct device *dev)
{
  /* Nothing to do yet */
  //struct proput_dev_data *pdd = dev_get_drvdata(dev);
  return 0;
}

static int proput_resume(struct device *dev)
{
  /* Nothing to do yet */
  //struct proput_dev_data *pdd = dev_get_drvdata(dev);
  return 0;
}

static SIMPLE_DEV_PM_OPS(proput_pm, proput_suspend, proput_resume);
#define PROPUT_PM	(&proput_pm)
#else
#define PROPUT_PM	NULL
#endif

static const struct of_device_id of_proput_match[] = {
	{ .compatible = "proput", },
	{},
};

MODULE_DEVICE_TABLE(of, of_proput_match);


static struct platform_driver proput_driver = {
  .probe = proput_probe,
  .shutdown = proput_shutdown,
  .driver = {
    .name = "proput",
    .pm = PROPUT_PM,
    .of_match_table = of_match_ptr(of_proput_match),
  },
};

int __init proput_init(void)
{
  int e;

  if ((e = ufp_init()))
    return e;

  if ((e = platform_driver_register(&proput_driver)))
  {
    ufp_exit();
    return e;
  }

  printk(KERN_INFO "ProPuT inserted\n");
  return 0;
}

void __exit proput_exit(void)
{
  ufp_exit();
  platform_driver_unregister(&proput_driver);
  printk(KERN_INFO "ProPuT removed\n");
}

module_init(proput_init);
module_exit(proput_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Maria Matejka <mq@jmq.cz>");
MODULE_DESCRIPTION("ProPuT");
//MODULE_SUPPORTED_DEVICE("proput");

