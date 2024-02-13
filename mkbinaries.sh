cd binutils
make install
cd bin
mkdir -p sirius
for f in * ; do cp -- "$f" "sirius/sirius-$f" ; done
cd ../../lcc
cd ..
