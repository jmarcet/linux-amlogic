/*
 * aml_g9tv.c  --  amlogic  G9TV sound card machine driver code.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/delay.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/jack.h>

#include <asm/mach-types.h>
#include <mach/hardware.h>

#include <linux/switch.h>
#include <linux/amlogic/saradc.h>
#include <linux/clk.h>

#include "aml_i2s_dai.h"
#include "aml_i2s.h"
#include "aml_g9tv.h"
#include "aml_audio_hw.h"
#include <mach/register.h>

#ifdef CONFIG_USE_OF
#include <linux/of.h>
#include <linux/pinctrl/consumer.h>
#include <linux/amlogic/aml_gpio_consumer.h>
#include <linux/amlogic/aml_audio_codec_probe.h>
#include <linux/of_gpio.h>
#include <mach/pinmux.h>
#include <plat/io.h>
#endif

#define DRV_NAME "aml_snd_g9tv"
extern int ext_codec;
extern struct device *spdif_dev;

static int aml_asoc_hw_params(struct snd_pcm_substream *substream,
    struct snd_pcm_hw_params *params)
{
    struct snd_soc_pcm_runtime *rtd = substream->private_data;
    struct snd_soc_dai *codec_dai = rtd->codec_dai;
    struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
    int ret;

    printk(KERN_DEBUG "enter %s stream: %s rate: %d format: %d\n", __func__, 
        (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) ? "playback" : "capture", 
        params_rate(params), params_format(params));

    /* set codec DAI configuration */
    ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S |
        SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
    if (ret < 0) {
        printk(KERN_ERR "%s: set codec dai fmt failed!\n", __func__);
        return ret;
    }

    /* set cpu DAI configuration */
    ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S |
        SND_SOC_DAIFMT_IB_NF | SND_SOC_DAIFMT_CBM_CFM);
    if (ret < 0) {
        printk(KERN_ERR "%s: set cpu dai fmt failed!\n", __func__);
        return ret;
    }
    
    if(!strncmp(codec_info.name_bus,"dummy_codec",11)){
        goto cpu_dai;
    }

    /* set codec DAI clock */
    ret = snd_soc_dai_set_sysclk(codec_dai, 0, params_rate(params) * 256, SND_SOC_CLOCK_IN);
    if (ret < 0) {
        printk(KERN_ERR "%s: set codec dai sysclk failed (rate: %d)!\n", __func__, params_rate(params));
        return ret;
    }
    
cpu_dai:
    /* set cpu DAI clock */
    ret = snd_soc_dai_set_sysclk(cpu_dai, 0, params_rate(params) * 256, SND_SOC_CLOCK_OUT);
    if (ret < 0) {
        printk(KERN_ERR "%s: set cpu dai sysclk failed (rate: %d)!\n", __func__, params_rate(params));
        return ret;
    }

    return 0;
}

static struct snd_soc_ops aml_asoc_ops = {
    .hw_params = aml_asoc_hw_params,
};

struct aml_audio_private_data *p_audio;
static bool aml_audio_i2s_mute_flag = 0;
static bool aml_audio_spdif_mute_flag = 0;
static int aml_audio_Hardware_resample = 0;


static int aml_audio_set_i2s_mute(struct snd_kcontrol *kcontrol,
    struct snd_ctl_elem_value *ucontrol)
{
    aml_audio_i2s_mute_flag = ucontrol->value.integer.value[0];
    printk("aml_audio_i2s_mute_flag: flag=%d\n",aml_audio_i2s_mute_flag);
    if(aml_audio_i2s_mute_flag){
        aml_audio_i2s_mute();
    }else{
        aml_audio_i2s_unmute();
    }
    return 0;
}

static int aml_audio_get_i2s_mute(struct snd_kcontrol *kcontrol,
    struct snd_ctl_elem_value *ucontrol)
{
    ucontrol->value.integer.value[0] = aml_audio_i2s_mute_flag;
    return 0;
}

static int aml_audio_set_spdif_mute(struct snd_kcontrol *kcontrol,
    struct snd_ctl_elem_value *ucontrol)
{
    
    aml_audio_spdif_mute_flag = ucontrol->value.integer.value[0];
    printk("aml_audio_set_spdif_mute: flag=%d\n",aml_audio_spdif_mute_flag);
    if(aml_audio_spdif_mute_flag){
        aml_spdif_pinmux_deinit(spdif_dev);
    }else{
        aml_spdif_pinmux_init(spdif_dev);
    }
    return 0;
}

static int aml_audio_get_spdif_mute(struct snd_kcontrol *kcontrol,
    struct snd_ctl_elem_value *ucontrol)
{
    ucontrol->value.integer.value[0] = aml_audio_spdif_mute_flag;
    return 0;
}

static const char *audio_in_source_texts[] = {
    "LINEIN", "ATV", "HDMI"
};

static const struct soc_enum audio_in_source_enum =
    SOC_ENUM_SINGLE(SND_SOC_NOPM, 0,
            ARRAY_SIZE(audio_in_source_texts),
            audio_in_source_texts);

static int aml_audio_get_in_source(struct snd_kcontrol *kcontrol,
    struct snd_ctl_elem_value *ucontrol)
{
    int value = READ_MPEG_REG(AUDIN_SOURCE_SEL) & 0x3;
    
    if (value == 0)
        ucontrol->value.enumerated.item[0] = 0;// linein
    else if (value == 1)
        ucontrol->value.enumerated.item[0] = 1;//ATV
    else if (value == 2)
        ucontrol->value.enumerated.item[0] = 2;//hdmi
    else
        printk(KERN_INFO "unknown source\n");
    
    return 0;
}

static int aml_audio_set_in_source(struct snd_kcontrol *kcontrol,
    struct snd_ctl_elem_value *ucontrol)
{
    if (ucontrol->value.enumerated.item[0] == 0){// select external codec output as I2S source
        WRITE_MPEG_REG(AUDIN_SOURCE_SEL, 0);
        WRITE_MPEG_REG(AUDIN_I2SIN_CTRL,(1<<I2SIN_CHAN_EN)
                                    |(3<<I2SIN_SIZE)
                                    |(1<<I2SIN_LRCLK_INVT)
                                    |(1<<I2SIN_LRCLK_SKEW)
                                    |(0<<I2SIN_POS_SYNC)
                                    |(1<<I2SIN_LRCLK_SEL)
                                    |(1<<I2SIN_CLK_SEL)
                                    |(1<<I2SIN_DIR));
        WRITE_MPEG_REG_BITS(AUDIN_I2SIN_CTRL, 1, I2SIN_EN, 1);
    }else if (ucontrol->value.enumerated.item[0] == 1){// select ATV output as I2S source
        WRITE_MPEG_REG(AUDIN_SOURCE_SEL, 1);
        WRITE_MPEG_REG(AUDIN_I2SIN_CTRL,(1<<I2SIN_CHAN_EN)
                                    |(0<<I2SIN_SIZE)
                                    |(0<<I2SIN_LRCLK_INVT)
                                    |(0<<I2SIN_LRCLK_SKEW)
                                    |(0<<I2SIN_POS_SYNC)
                                    |(0<<I2SIN_LRCLK_SEL)
                                    |(0<<I2SIN_CLK_SEL)
                                    |(0<<I2SIN_DIR));
        WRITE_MPEG_REG_BITS(AUDIN_I2SIN_CTRL, 1, I2SIN_EN, 1);
    }else if (ucontrol->value.enumerated.item[0] == 2){// select HDMI-rx as I2S source
        WRITE_MPEG_REG(AUDIN_SOURCE_SEL, (0  <<12)| // [14:12]cntl_hdmirx_chsts_sel: 0=Report chan1 status; 1=Report chan2 status; ...;
                                         (0xf<< 8)| // [11:8] cntl_hdmirx_chsts_en
                                         (1  << 4)| // [5:4]  spdif_src_sel: 1=Select HDMIRX SPDIF output as AUDIN source
                                         (2  << 0)); // [1:0]  i2sin_src_sel: 2=Select HDMIRX I2S output as AUDIN source
        WRITE_MPEG_REG(AUDIN_I2SIN_CTRL,(1<<I2SIN_CHAN_EN)
                                    |(3<<I2SIN_SIZE)
                                    |(1<<I2SIN_LRCLK_INVT)
                                    |(1<<I2SIN_LRCLK_SKEW)
                                    |(0<<I2SIN_POS_SYNC)
                                    |(1<<I2SIN_LRCLK_SEL)
                                    |(1<<I2SIN_CLK_SEL)
                                    |(1<<I2SIN_DIR));
        WRITE_MPEG_REG_BITS(AUDIN_I2SIN_CTRL, 1, I2SIN_EN, 1);
    }
    return 0;
}

/* HDMI audio format detect: LPCM or NONE-LPCM */           
static const char *hdmi_audio_type_texts[] = {
    "LPCM","NONE-LPCM","UN-KNOWN"
};          
static const struct soc_enum hdmi_audio_type_enum =
    SOC_ENUM_SINGLE(SND_SOC_NOPM, 0,
            ARRAY_SIZE(hdmi_audio_type_texts),
            hdmi_audio_type_texts);

static int aml_hdmi_audio_type_get_enum(struct snd_kcontrol *kcontrol,
    struct snd_ctl_elem_value *ucontrol)
{
    int ch_status = 0;
    if ((READ_MPEG_REG(AUDIN_DECODE_CONTROL_STATUS)>>24)&0x1){
        ch_status = READ_MPEG_REG(AUDIN_DECODE_CHANNEL_STATUS_A_0);
        if (ch_status&2) //NONE-LPCM
            ucontrol->value.enumerated.item[0] = 1;
        else //LPCM
            ucontrol->value.enumerated.item[0] = 0;     
    }
    else
        ucontrol->value.enumerated.item[0] = 2; //un-stable. un-known       
    
    return 0;
}

static int aml_hdmi_audio_type_set_enum(struct snd_kcontrol *kcontrol,
    struct snd_ctl_elem_value *ucontrol)
{
    return 0;
}

/* spdif in audio format detect: LPCM or NONE-LPCM */
static const char *spdif_audio_type_texts[] = {
    "LPCM","NONE-LPCM","UN-KNOWN"
};          
static const struct soc_enum spdif_audio_type_enum =
    SOC_ENUM_SINGLE(SND_SOC_NOPM, 0,
            ARRAY_SIZE(spdif_audio_type_texts),
            spdif_audio_type_texts);

static int aml_spdif_audio_type_get_enum(struct snd_kcontrol *kcontrol,
    struct snd_ctl_elem_value *ucontrol)
{
	int ch_status = 0;
    //if ((READ_MPEG_REG(AUDIN_SPDIF_MISC)>>0x7)&0x1){
        ch_status = READ_MPEG_REG(AUDIN_SPDIF_CHNL_STS_A)&0x3;
        if (ch_status&2) //NONE-LPCM
            ucontrol->value.enumerated.item[0] = 1;
        else //LPCM
            ucontrol->value.enumerated.item[0] = 0;     
    //}
    //else
    //    ucontrol->value.enumerated.item[0] = 2; //un-stable. un-known       
    return 0;
}

static int aml_spdif_audio_type_set_enum(struct snd_kcontrol *kcontrol,
    struct snd_ctl_elem_value *ucontrol)
{
    return 0;
}

#define RESAMPLE_BUFFER_SOURCE 1 //audioin fifo0
#define RESAMPLE_CNT_CONTROL 255 //Cnt_ctrl = mclk/fs_out-1 ; fest 256fs

static int hardware_resample_enable(int input_sr){
    struct clk* clk_src;
    unsigned long clk_rate = 0;
    unsigned short Avg_cnt_init = 0;
    
    if(input_sr < 8000 || input_sr > 48000){
        printk(KERN_INFO "Error input sample rate, you must set sr first!\n");
        return -1;
    }

    clk_src = clk_get_sys("clk81", NULL);  // get clk81 clk_rate
    clk_rate = clk_get_rate(clk_src);
    Avg_cnt_init = (unsigned short)(clk_rate*4/input_sr);

    printk(KERN_INFO "clk_rate = %ld,input_sr = %d,Avg_cnt_init = %d\n",clk_rate,input_sr,Avg_cnt_init);
	
    WRITE_MPEG_REG(AUD_RESAMPLE_CTRL0, 0);
    WRITE_MPEG_REG(AUD_RESAMPLE_CTRL0,  (0<<29)    //select the source [30:29]
                                        |(1<<28)   //enable the resample [28]
                                        |(0<<26)   //select the method 0 [27:26]
                                        |(RESAMPLE_CNT_CONTROL<<16)  //calculate the cnt_ctrl [25:16]
                                        | Avg_cnt_init);   //calculate the avg_cnt_init [15:0]
    
    return 0;
}

static int hardware_resample_disable(void){

    WRITE_MPEG_REG(AUD_RESAMPLE_CTRL0, 0);
    return 0;
}

static const char *hardware_resample_texts[] = {
    "Disable", "Enable:48K", "Enable:44K","Enable:32K"
};

static const struct soc_enum hardware_resample_enum =
    SOC_ENUM_SINGLE(SND_SOC_NOPM, 0,
            ARRAY_SIZE(hardware_resample_texts),
            hardware_resample_texts);

static int aml_hardware_resample_get_enum(struct snd_kcontrol *kcontrol,
    struct snd_ctl_elem_value *ucontrol)
{
    ucontrol->value.enumerated.item[0] = aml_audio_Hardware_resample;
    return 0;
}

static int aml_hardware_resample_set_enum(struct snd_kcontrol *kcontrol,
    struct snd_ctl_elem_value *ucontrol)
{
    if (ucontrol->value.enumerated.item[0] == 0){
        hardware_resample_disable();
        aml_audio_Hardware_resample = 0;
    }else if (ucontrol->value.enumerated.item[0] == 1){
        hardware_resample_enable(48000);
        aml_audio_Hardware_resample = 1;
    }else if (ucontrol->value.enumerated.item[0] == 2){
        hardware_resample_enable(44100);
        aml_audio_Hardware_resample = 2;
    }else if (ucontrol->value.enumerated.item[0] == 3){
        hardware_resample_enable(32000);
        aml_audio_Hardware_resample = 3;
    }
    return 0;
}

static int aml_set_bias_level(struct snd_soc_card *card,
        struct snd_soc_dapm_context *dapm, enum snd_soc_bias_level level)
{
    //printk(KERN_INFO "enter %s\n", __func__);
    return 0;
}

static const struct snd_soc_dapm_widget aml_asoc_dapm_widgets[] = {
    SND_SOC_DAPM_INPUT("LINEIN"),
    SND_SOC_DAPM_OUTPUT("LINEOUT"),
};

static const struct snd_kcontrol_new aml_g9tv_controls[] = {

    SOC_SINGLE_BOOL_EXT("Audio i2s mute", 0,
        aml_audio_get_i2s_mute,
        aml_audio_set_i2s_mute),
        
    SOC_SINGLE_BOOL_EXT("Audio spdif mute", 0,
        aml_audio_get_spdif_mute,
        aml_audio_set_spdif_mute),

    SOC_ENUM_EXT("Audio In Source", audio_in_source_enum,
        aml_audio_get_in_source,
        aml_audio_set_in_source),

    SOC_ENUM_EXT("HDMI Audio Type", hdmi_audio_type_enum,
        aml_hdmi_audio_type_get_enum,
        aml_hdmi_audio_type_set_enum),

    SOC_ENUM_EXT("SPDIFIN Audio Type", spdif_audio_type_enum,
        aml_spdif_audio_type_get_enum,
        aml_spdif_audio_type_set_enum),
    
    SOC_ENUM_EXT("Hardware resample enable", hardware_resample_enum,
        aml_hardware_resample_get_enum,
        aml_hardware_resample_set_enum),
    
};

static int aml_asoc_init(struct snd_soc_pcm_runtime *rtd)
{
    struct snd_soc_card *card = rtd->card;
    struct snd_soc_codec *codec = rtd->codec;
    struct snd_soc_dapm_context *dapm = &codec->dapm;
    struct aml_audio_private_data * p_aml_audio;
    int ret = 0;;
    
    printk(KERN_DEBUG "enter %s \n", __func__);
    p_aml_audio = snd_soc_card_get_drvdata(card);
    ret = snd_soc_add_card_controls(codec->card, aml_g9tv_controls,
                ARRAY_SIZE(aml_g9tv_controls));
    if (ret)
       return ret;
    /* Add specific widgets */
    snd_soc_dapm_new_controls(dapm, aml_asoc_dapm_widgets,
                  ARRAY_SIZE(aml_asoc_dapm_widgets));

    return 0;
}

static struct snd_soc_dai_link aml_codec_dai_link[] = {
    {
        .name = "SND_G9TV",
        .stream_name = "AML PCM",
        .cpu_dai_name = "aml-i2s-dai.0",
        .init = aml_asoc_init,
        .platform_name = "aml-i2s.0",
        .ops = &aml_asoc_ops,
    },
#ifdef CONFIG_SND_SOC_PCM2BT
    {
        .name = "BT Voice",
        .stream_name = "Voice PCM",
        .cpu_dai_name = "aml-pcm-dai.0",
        .codec_dai_name = "pcm2bt-pcm",
        .platform_name = "aml-pcm.0",
        .codec_name = "pcm2bt.0",
    },
#endif
    {
        .name = "AML-SPDIF",
        .stream_name = "SPDIF PCM",
        .cpu_dai_name = "aml-spdif-dai.0",
        .codec_dai_name = "dit-hifi",
        .init = NULL,
        .platform_name = "aml-i2s.0",
        .codec_name = "spdif-dit.0",
        .ops = NULL,      
    },
};

static struct snd_soc_card aml_snd_soc_card = {
    .driver_name = "SOC-Audio",
    .dai_link = &aml_codec_dai_link[0],
    .num_links = ARRAY_SIZE(aml_codec_dai_link),
    .set_bias_level = aml_set_bias_level,
};

static void aml_g9tv_pinmux_init(struct snd_soc_card *card)
{
    struct aml_audio_private_data *p_aml_audio;
    const char *str=NULL;
    int ret;
    p_aml_audio = snd_soc_card_get_drvdata(card);   

    if (!IS_MESON_MG9TV_CPU_REVA)
    	p_aml_audio->pin_ctl = devm_pinctrl_get_select(card->dev, "aml_snd_g9tv");
    
    p_audio = p_aml_audio;
    printk(KERN_INFO "audio external codec = %d\n",ext_codec);

    ret = of_property_read_string(card->dev->of_node, "mute_gpio", &str);
    if (ret < 0) {
        printk("aml_snd_g9tv: faild to get mute_gpio!\n");
    }else{
        p_aml_audio->gpio_mute = amlogic_gpio_name_map_num(str);
        p_aml_audio->mute_inv = of_property_read_bool(card->dev->of_node,"mute_inv");
        amlogic_gpio_request_one(p_aml_audio->gpio_mute,GPIOF_OUT_INIT_LOW,"mute_spk");
        amlogic_set_value(p_aml_audio->gpio_mute, 0, "mute_spk");
    }
}

static void aml_g9tv_pinmux_deinit(struct snd_soc_card *card)
{
    struct aml_audio_private_data *p_aml_audio;

    p_aml_audio = snd_soc_card_get_drvdata(card);
    if(p_aml_audio->pin_ctl)
        devm_pinctrl_put(p_aml_audio->pin_ctl);
}

static int aml_g9tv_audio_probe(struct platform_device *pdev)
{
    struct snd_soc_card *card = &aml_snd_soc_card;
    struct aml_audio_private_data *p_aml_audio;
    char tmp[NAME_SIZE];
    int ret = 0;

#ifdef CONFIG_USE_OF
    p_aml_audio = devm_kzalloc(&pdev->dev,
            sizeof(struct aml_audio_private_data), GFP_KERNEL);
    if (!p_aml_audio) {
        dev_err(&pdev->dev, "Can't allocate aml_audio_private_data\n");
        ret = -ENOMEM;
        goto err;
    }

    card->dev = &pdev->dev;
    platform_set_drvdata(pdev, card);
    snd_soc_card_set_drvdata(card, p_aml_audio);
    if (!(pdev->dev.of_node)) {
        dev_err(&pdev->dev, "Must be instantiated using device tree\n");
        ret = -EINVAL;
        goto err;
    }

    ret = snd_soc_of_parse_card_name(card, "aml,sound_card");
    if (ret)
        goto err;

    ret = of_property_read_string_index(pdev->dev.of_node, "aml,codec_dai",
            codec_info.codec_index, &aml_codec_dai_link[0].codec_dai_name);
    if (ret)
        goto err;

    printk("codec_name = %s\n", codec_info.name_bus);

    aml_codec_dai_link[0].codec_name = codec_info.name_bus;
    
    snprintf(tmp, NAME_SIZE, "%s-%s", "aml,audio-routing", codec_info.name);

    ret = snd_soc_of_parse_audio_routing(card, tmp);
    if (ret)
        goto err;

    ret = snd_soc_register_card(card);
    if (ret) {
        dev_err(&pdev->dev, "snd_soc_register_card failed (%d)\n",
            ret);
        goto err;
    }

    aml_g9tv_pinmux_init(card);
    return 0;
#endif

err:
    kfree(p_aml_audio);
    return ret;
}

static int aml_g9tv_audio_remove(struct platform_device *pdev)
{
    int ret = 0;
    struct snd_soc_card *card = platform_get_drvdata(pdev);
    struct aml_audio_private_data *p_aml_audio;

    p_aml_audio = snd_soc_card_get_drvdata(card);
    snd_soc_unregister_card(card);
    aml_g9tv_pinmux_deinit(card);
    kfree(p_aml_audio);
    return ret;
}

#ifdef CONFIG_USE_OF
static const struct of_device_id amlogic_audio_dt_match[]={
    { .compatible = "sound_card, aml_snd_g9tv", },
    {},
};
#else
#define amlogic_audio_dt_match NULL
#endif

static struct platform_driver aml_g9tv_audio_driver = {
    .probe  = aml_g9tv_audio_probe,
    .remove = aml_g9tv_audio_remove,
    .driver = {
        .name = DRV_NAME,
        .owner = THIS_MODULE,
        .pm = &snd_soc_pm_ops,
        .of_match_table = amlogic_audio_dt_match,
    },
};

static int __init aml_g9tv_audio_init(void)
{
    return platform_driver_register(&aml_g9tv_audio_driver);
}

static void __exit aml_g9tv_audio_exit(void)
{
    platform_driver_unregister(&aml_g9tv_audio_driver);
}

module_init(aml_g9tv_audio_init);
module_exit(aml_g9tv_audio_exit);

/* Module information */
MODULE_AUTHOR("AMLogic, Inc.");
MODULE_DESCRIPTION("AML_G9TV audio machine Asoc driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" DRV_NAME);
MODULE_DEVICE_TABLE(of, amlogic_audio_dt_match);

