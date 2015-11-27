rm symbols.c symbols.h
make clean
make contiki_exercise12.sky
make contiki_exercise12.sky CORE=contiki_exercise12.sky
make contiki_exercise12.upload CORE=contiki_exercise12.sky $1

