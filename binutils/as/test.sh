customasm master.asm t.s -f binary -o custom.bin -q
./as t.s > /dev/null 2>&1
echo -n "\e[4;31m"
diff a.out custom.bin
cmp -l a.out custom.bin | gawk '{printf "%08X %02X %02X\n", $1, strtonum(0$2), strtonum(0$3)}'
echo -n "\033[0m"
xxd a.out
echo "---"
xxd custom.bin
