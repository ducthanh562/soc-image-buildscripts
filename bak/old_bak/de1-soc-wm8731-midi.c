/*
* de1-soc-wm8731 -- SoC audio for Terasic DE1-SoC board
* Author: B. Steinsbo <bsteinsbo@gmail.com>
*
* Based on sam9g20_wm8731 by
* Sedji Gaouaou <sedji.gaouaou@atmel.com>
*
* Licensed under the GPL-2.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/uaccess.h>
#include <linux/ioport.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>

#include <sound/core.h>
#include <sound/seq_kernel.h>
#include <sound/rawmidi.h>
#include <sound/initval.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include "../codecs/wm8731.h"

#define SYNTHSOCSOUND_LOG_PREFIX "synthsound: "

//#define HSYNTH_SYSCLK_MCLK 2

#define MCLK_RATE_48K 12288000 /* fs*256 */
#define MCLK_RATE_44K 16934400 /* fs*384 */

#define midi_ins 0
#define midi_outs 1

#define MIDIREG_BASE 0xff200000
#define MIDIREG_SIZE PAGE_SIZE
#define MIDIREG_OFFSET 0x50000

#define printe(...) pr_err(SYNTHSOCSOUND_LOG_PREFIX __VA_ARGS__)

static unsigned int i2c_mux_gpio;

static int snd_socmidi_open(struct snd_rawmidi_substream *substream);
static int snd_socmidi_close(struct snd_rawmidi_substream *substream);
static int hmidi_card_probe(struct snd_soc_card *card);
static int hmidi_card_remove(struct snd_soc_card *card);
static void snd_socmidi_transmit(unsigned char data);
static void snd_socmidi_output_trigger(struct snd_rawmidi_substream *substream, int);
static int hsound_midi_init(struct snd_card *card);
static int soc_hsynth_init(struct snd_soc_pcm_runtime *rtd);

void *midireg_mem;

static int snd_socmidi_open(struct snd_rawmidi_substream *substream)
{
    return 0;
}

static int snd_socmidi_close(struct snd_rawmidi_substream *substream)
{
    return 0;
}

static int hmidi_card_probe(struct snd_soc_card *card)
{
    int err;

    err = hsound_midi_init(card->snd_card);

    if (err < 0) {
        //		dev_dbg(&pdev->dev,"hsound_midi_init failed: %d\n", err);
        return err;
    }

    return 0;
}

static int hmidi_card_remove(struct snd_soc_card *card)
{
    return 0;
}

static void snd_socmidi_transmit(unsigned char data){

    iowrite8(data, midireg_mem);

}

static void snd_socmidi_output_trigger(struct snd_rawmidi_substream *substream, int up) {

    if (!up)
        return;

    while (1) {
        unsigned char data;
        if (snd_rawmidi_transmit(substream, &data, 1) != 1)
            break; /* no more data */
            snd_socmidi_transmit(data);
    }
}

static struct snd_rawmidi *g_rmidi;

static struct snd_rawmidi_ops snd_socmidi_output_ops = {
    .open = snd_socmidi_open,
    .close = snd_socmidi_close,
    .trigger = snd_socmidi_output_trigger,
};

static void pisnd_get_port_info(struct snd_rawmidi *rmidi, int number, struct snd_seq_port_info *seq_port_info)
{
    seq_port_info->type =
    SNDRV_SEQ_PORT_TYPE_MIDI_GENERIC |
    SNDRV_SEQ_PORT_TYPE_HARDWARE |
    SNDRV_SEQ_PORT_TYPE_PORT;
    seq_port_info->midi_voices = 0;
}

static struct snd_rawmidi_global_ops hsnd_global_ops = {.get_port_info = pisnd_get_port_info, };

static int hsound_midi_init(struct snd_card *card)
{
    int err;
    struct resource *res;

    err = snd_rawmidi_new(card, "SocMIDI", 0, midi_outs, midi_ins, &g_rmidi);

    if (err < 0) {
        printe("snd_rawmidi_new failed: %d\n", err);
        return err;
    }

    strcpy(g_rmidi->name, "Holosynth MIDI ");
    //	strcat(g_rmidi->name, pisnd_spi_get_serial());

    g_rmidi->info_flags = SNDRV_RAWMIDI_INFO_OUTPUT;
    // rmidi->info_flags = SNDRV_RAWMIDI_INFO_OUTPUT |
    //                     SNDRV_RAWMIDI_INFO_INPUT |
    //                     SNDRV_RAWMIDI_INFO_DUPLEX;

    g_rmidi->ops = &hsnd_global_ops;

    g_rmidi->private_data = (void *)0;

    snd_rawmidi_set_ops(g_rmidi, SNDRV_RAWMIDI_STREAM_OUTPUT, &snd_socmidi_output_ops);
    //    snd_rawmidi_set_ops(rmidi, SNDRV_RAWMIDI_STREAM_INPUT, &snd_socmidi_input_ops);

    res = request_mem_region((MIDIREG_BASE + MIDIREG_OFFSET), MIDIREG_SIZE, "MIDIREG");
    if (res == NULL) {
        return -EBUSY;
    }

    midireg_mem = ioremap((MIDIREG_BASE + MIDIREG_OFFSET), MIDIREG_SIZE);
    if (midireg_mem == NULL) {
        release_mem_region(MIDIREG_BASE, MIDIREG_SIZE);
        return -EFAULT;
    }

    return 0;
}

static int soc_hsynth_init(struct snd_soc_pcm_runtime *rtd)
{
    struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
    struct device *dev = rtd->card->dev;
    unsigned int fmt;
    int ret;

    dev_dbg(dev, "init\n");

    fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
    SND_SOC_DAIFMT_CBS_CFS;

    /* set cpu DAI configuration */
    ret = snd_soc_dai_set_fmt(cpu_dai, fmt);
    if (ret < 0)
        return ret;

    return 0;
}

static int de1soc_hw_params(struct snd_pcm_substream *substream,
    struct snd_pcm_hw_params *params)
{
    struct snd_soc_pcm_runtime *rtd = substream->private_data;
    struct snd_soc_dai *codec_dai = rtd->codec_dai;
    struct device *dev = rtd->card->dev;
    unsigned int mclk_freq;
    int ret;

    if ((params_rate(params) % 44100) == 0) {
        mclk_freq = MCLK_RATE_44K;
    } else if ((params_rate(params) % 48000) == 0) {
        mclk_freq = MCLK_RATE_48K;
    } else
        return -EINVAL;

    /* set codec mclk configuration */
    ret = snd_soc_dai_set_sysclk(codec_dai, WM8731_SYSCLK_MCLK,
        mclk_freq, SND_SOC_CLOCK_OUT);
    if (ret < 0)
        return ret;

    dev_dbg(dev, "hw_params: mclk_freq=%d\n", mclk_freq);
    return 0;
}

static void de1soc_shutdown(struct snd_pcm_substream *substream)
{
    struct snd_soc_pcm_runtime *rtd = substream->private_data;
    struct snd_soc_dai *codec_dai = rtd->codec_dai;
    struct device *dev = rtd->card->dev;
    int ret;

    dev_dbg(dev, "shutdown\n");
    ret = snd_soc_dai_set_sysclk(codec_dai, WM8731_SYSCLK_MCLK,
        0, SND_SOC_CLOCK_OUT);
    if (ret < 0) {
        dev_err(dev, "Failed to reset WM8731 SYSCLK: %d\n", ret);
    }
}

static struct snd_soc_ops de1soc_ops = {
    // .startup
    .shutdown = de1soc_shutdown,
    .hw_params = de1soc_hw_params,
    // .hw_free
    // .prepare
    // .trigger
};

static const struct snd_soc_dapm_widget de1soc_dapm_widgets[] = {
    SND_SOC_DAPM_HP("Headphone Jack", NULL),
    SND_SOC_DAPM_MIC("Microphone Jack", NULL),
    SND_SOC_DAPM_LINE("Line In Jack", NULL),
    SND_SOC_DAPM_LINE("Line Out Jack", NULL),
//    SND_SOC_DAPM_LINE("Hsynth in", NULL),
};

static const struct snd_soc_dapm_route intercon[] = {
    {"MICIN", NULL, "Mic Bias"},
    {"Mic Bias", NULL, "Microphone Jack"},
    {"LLINEIN", NULL, "Line In Jack"},
    {"RLINEIN", NULL, "Line In Jack"},
    {"Line Out Jack", NULL, "LOUT"},
    {"Line Out Jack", NULL, "ROUT"},
    {"Headphone Jack", NULL, "LHPOUT"},
    {"Headphone Jack", NULL, "RHPOUT"},
//    {"PDM_DAT", NULL, "Hsynth in"},
};


static int de1soc_wm8731_init(struct snd_soc_pcm_runtime *rtd)
{
    struct snd_soc_dai *codec_dai = rtd->codec_dai;
    struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
    struct device *dev = rtd->card->dev;
    unsigned int fmt;
    int ret;

    dev_dbg(dev, "init\n");

    fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
    SND_SOC_DAIFMT_CBS_CFS;

    ret = snd_soc_dai_set_fmt(cpu_dai, fmt);
    if (ret < 0)
        return ret;

    ret = snd_soc_dai_set_fmt(codec_dai, fmt);
    if (ret < 0)
        return ret;

    // Don't let codec constraints interfere
    ret = snd_soc_dai_set_sysclk(codec_dai, WM8731_SYSCLK_MCLK,
                                 0, SND_SOC_CLOCK_OUT);
    if (ret < 0) {
        dev_err(dev, "Failed to set WM8731 SYSCLK: %d\n", ret);
        return ret;
    }

    return 0;
}

#define DE1_SOC_DAI_FMT (SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | \
SND_SOC_DAIFMT_CBS_CFS)
/*
static struct snd_soc_dai_link de1soc_dai[] = {
    {
        .name = "WM8731 Tx",
        .stream_name = "WM8731 PCM Play",
        .cpu_dai_name = "ff200000.i2s",
        .codec_dai_name = "wm8731-hifi",
        .platform_name = "de1soc",
        .codec_name = "wm8731.0-001a",
        .init = de1soc_wm8731_init,
        .ops = &de1soc_ops,
        .dai_fmt = DE1_SOC_DAI_FMT,
    },
};
*/

enum {
    DAI_LINK_PLAYBACK,
    DAI_LINK_CAPTURE,
//    DAI_LINK_HSYNTH,
};

static struct snd_soc_dai_link de1soc_dai[] = {
    [DAI_LINK_PLAYBACK] = {
        .name = "WM8731 Tx",
        .stream_name = "WM8731 PCM Play",
        .cpu_dai_name = "ff200000.i2s",
        .codec_dai_name = "wm8731-hifi",
        .platform_name = "de1soc",
        .codec_name = "wm8731.0-001a",
//        .init = de1soc_wm8731_init,
        .dai_fmt = DE1_SOC_DAI_FMT,
        .ops = &de1soc_ops,
        .playback_only	= true,
    },
    [DAI_LINK_CAPTURE] = {
        .name = "WM8731 Rx",
        .stream_name = "WM8731 PCM Cap",
        .cpu_dai_name = "ff200000.i2s",
        .codec_dai_name = "wm8731-hifi",
//        .init = de1soc_wm8731_init,
        .platform_name = "de1soc",
        .codec_name = "wm8731.0-001a",
        .dai_fmt = DE1_SOC_DAI_FMT,
        .ops = &de1soc_ops,
        .capture_only	= true,
    },/*
    [DAI_LINK_HSYNTH] = {
        .name = "HSYNTH",
        .stream_name = "HSYNTH PCM",
        .cpu_dai_name = "ff200000.dmalink",
        .codec_dai_name = "hsynth7002-hifi",
        .platform_name = "de1soc",
        .codec_name = "hsynth7002.hsynth",
        .dai_fmt = DE1_SOC_DAI_FMT,
        .ops = &de1soc_ops,
        .capture_only	= true,
    },*/
};

static struct snd_soc_card snd_soc_de1soc = {
    .name = "DE1-HSYNTH-WM8731",
    .owner = THIS_MODULE,
    .dai_link = de1soc_dai,
    .num_links = ARRAY_SIZE(de1soc_dai),
//    .num_links = 1,

    .probe        = hmidi_card_probe,
    .remove       = hmidi_card_remove,

    .dapm_widgets = de1soc_dapm_widgets,
    .num_dapm_widgets = ARRAY_SIZE(de1soc_dapm_widgets),
    .dapm_routes = intercon,
    .num_dapm_routes = ARRAY_SIZE(intercon),
};

static int de1soc_audio_probe(struct platform_device *pdev)
{
    struct snd_soc_card *card = &snd_soc_de1soc;
    int ret, i;
//    int ret;
    struct device_node *np = pdev->dev.of_node;
    struct device_node  *cpu_np[2], *codec_np[2];
//    struct device_node  *cpu_np[3], *codec_np[3];

    if (!np) {
        return -ENODEV;
    }

    /* I2C bus is muxed between HPS and FPGA. Set mux to HPS */
    i2c_mux_gpio = of_get_named_gpio(np, "i2c-mux-gpio", 0);
    if (gpio_is_valid(i2c_mux_gpio)) {
        ret = devm_gpio_request_one(&pdev->dev,
            i2c_mux_gpio, GPIOF_OUT_INIT_LOW, "I2C_MUX");
        if (ret) {
            dev_err(&pdev->dev,
                "Failed to request GPIO_%d for i2c_mux: %d\n",
                i2c_mux_gpio, ret);
            return ret;
        }
        gpio_set_value(i2c_mux_gpio, 1);
    }

    /* Parse codec info */
//      cpu_np = of_parse_phandle(np, "i2s-controller", 0);
    cpu_np[0] = of_parse_phandle(np, "i2s-controller", 0);
    cpu_np[1] = of_parse_phandle(np, "i2s-controller", 1);
//     cpu_np[2] = of_parse_phandle(np, "dmalink-controller", 2);
    codec_np[0] = of_parse_phandle(np, "audio-codec", 0);
    codec_np[1] = of_parse_phandle(np, "audio-codec", 1);
//     codec_np[2] = of_parse_phandle(np, "audio-codec", 2);
    //     if (!cpu_np[0] || !cpu_np[1] || !codec_np) {
    //         dev_err(&pdev->dev, "codec phandle info missing\n");
    //         return -EINVAL;
//     }
//      de1soc_dai[0].codec_name = NULL;
//      de1soc_dai[0].codec_of_node = codec_np;
//      de1soc_dai[0].cpu_dai_name = NULL;
//      de1soc_dai[0].cpu_of_node = cpu_np[0];
//      de1soc_dai[0].platform_name = NULL;
//      de1soc_dai[0].platform_of_node = cpu_np[0];

    for (i = 0; i < card->num_links; i++) {
        de1soc_dai[i].codec_name = NULL;
        de1soc_dai[i].codec_of_node = codec_np[i];
        de1soc_dai[i].cpu_dai_name = NULL;
        de1soc_dai[i].cpu_of_node = cpu_np[i];
        de1soc_dai[i].platform_name = NULL;
        de1soc_dai[i].platform_of_node = cpu_np[i];
        of_node_put(codec_np[i]);
        of_node_put(cpu_np[i]);
    }

//    of_node_put(codec_np);
    //    of_node_put(cpu_np);
//     of_node_put(cpu_np[0]);

    card->dev = &pdev->dev;
    platform_set_drvdata(pdev, card);

    ret = devm_snd_soc_register_card(&pdev->dev, card);
    if (ret) {
        dev_err(&pdev->dev, "snd_soc_register_card failed (%d)\n",
                ret);
    }

    return ret;
    /*
   ret = snd_soc_register_card(card);
   if (ret) {
       dev_err(&pdev->dev, "snd_soc_register_card() failed (%d)\n", ret);
       return ret;
   }

   return 0;
*/
}

static int de1soc_audio_remove(struct platform_device *pdev)
{
    struct snd_soc_card *card = platform_get_drvdata(pdev);

    if (gpio_is_valid(i2c_mux_gpio))
        devm_gpio_free(&pdev->dev, i2c_mux_gpio);
//    snd_soc_unregister_card(card);

    return 0;
}

static const struct of_device_id de1soc_wm8731_dt_ids[] = {
    { .compatible = "opencores,de1soc-wm8731-audio", },
//    { .compatible = "holotr,socsynth-audio", },
    { }
};

MODULE_DEVICE_TABLE(of, de1soc_wm8731_dt_ids);

static struct platform_driver de1soc_audio_driver = {
    .driver = {
        .name	= "de1soc-hsynth-audio",
//        .owner	= THIS_MODULE,
        .of_match_table = of_match_ptr(de1soc_wm8731_dt_ids),
    },
    .probe	= de1soc_audio_probe,
    .remove	= de1soc_audio_remove,
};

module_platform_driver(de1soc_audio_driver);

/* Module information */
MODULE_AUTHOR("Bjarne Steinsbo <bsteinsbo@gmail.com>");
MODULE_DESCRIPTION("ALSA SoC DE1-HSYNTH_WM8731");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:de1soc-hsynth-audio");
