mkdir 00000000
cd 00000000
mkdir temp
cd temp
echo Benjamin > Benjamin
echo Esterlis1 > Esterlis1 #username = lastname
echo esterlis > esterlis
cp Benjamin ../Esterlis1
cp Esterlis1 ../Benjamin
rm Benjamin Esterlis1
mv esterlis ../
cd ..
rmdir temp
echo 'content of the original directory:'
ls -la
echo 'content of the First name file:'
cat Benjamin
echo 'content of the  Last name file:'
cat Esterlis1
echo 'content of the  Tau username file:'
cat esterlis