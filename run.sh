#make valgrind ARGS="-d /home/brighton/media/manga/Tsubaki-chou_Lonely_Planet -o Tsubaki-chou_Lonely_Planet.cbz"
#make run ARGS="-v -c -f /home/brighton/media/manga/Tsubaki-chou_Lonely_Planet/[0001]_Chapter_1.cbz -o Tsubaki-chou_Lonely_Planet.cbz"
#make run ARGS="-f /home/brighton/media/manga/Tsubaki-chou_Lonely_Planet/[0001]_Chapter_1.cbz -o Tsubaki-chou_Lonely_Planet.pdf"
#make run ARGS="-f /home/brighton/media/manga/Vinland_Saga/[0001]_Chapter_1_Normanni.cbz -o vinland.pdf"
#make valgrind ARGS="-f /home/brighton/media/manga/Vinland_Saga/[0001]_Chapter_1_Normanni.cbz -o vinland.pdf"
#make run ARGS="-v -c -f ./t[01].cbz -o Tsubaki-chou_Lonely_Planet.pdf"
#make gdb ARGS="-v -c -f ./t[01].cbz -o Tsubaki-chou_Lonely_Planet.pdf"
#make run ARGS="-v -c -f ./custom[0001].cbz -o custom.pdf"
make run ARGS="-v -c -d ~/media/manga/Kamisama_Kiss -o reb.pdf"

#make run ARGS="-v -c -f ./custom[0001]-jpg.cbz -o custom-jpg.pdf" VAL_ARGS=" --leak-check=full -s  --track-origins=yes"
#echo -e '\n\n'
#make run ARGS="-v -c -f ./custom[0001]-png.cbz -o custom-png.pdf" VAL_ARGS=" --leak-check=full -s  --track-origins=yes"
