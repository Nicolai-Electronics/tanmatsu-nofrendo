idf_component_register(
    SRCS
        src/bitmap.c
        src/config.c
        src/event.c
        src/gui.c
        src/gui_elem.c
        src/intro.c
        src/log.c
        src/memguard.c
        src/nofrendo.c
        src/pcx.c
        src/vid_drv.c

        src/nes/mmclist.c
        src/nes/nes_mmc.c
        src/nes/nes_pal.c
        src/nes/nes_ppu.c
        src/nes/nes_rom.c
        src/nes/nes.c
        src/nes/nesinput.c
        src/nes/nesstate.c

        src/sndhrdw/fds_snd.c
        src/sndhrdw/mmc5_snd.c
        src/sndhrdw/nes_apu.c
        src/sndhrdw/vrcvisnd.c

        src/libsnss/libsnss.c

        src/esp32/osd.c
        # src/esp32/psxcontroller.c
        src/esp32/spi_lcd.c
        src/esp32/video_audio.c

        src/cpu/dis6502.c
        src/cpu/nes6502.c

        src/menu/src
    INCLUDE_DIRS
        "src"
        "src/nes"
        "src/sndhrdw"
        "src/libsnss"
        "src/esp32"
        "src/cpu"
        "../menu/src"
    PRIV_REQUIRES
        badge-bsp
)