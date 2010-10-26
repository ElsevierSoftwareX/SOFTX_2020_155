
all: frLongFile readframe tocLongFile readtoc maketoc

# Frame Files CC code
frLongFile: frLongFile.cc
	/ldcg/bin/gcc -o frLongFile -I/ldas/ldas-0.0/include -I/ldcg/include frLongFile.cc  -L/ldas/ldas-0.0/lib -L/ldcg/lib  -lgeneral -lframecpp -lz

readframe: readframe.cc
	/ldcg/bin/gcc -o readframe -I/ldas/ldas-0.0/include -I/ldcg/include readframe.cc  -L/ldas/ldas-0.0/lib -L/ldcg/lib  -lgeneral -lframecpp -lz

# TOC Files CC code
tocLongFile: tocLongFile.cc
	/ldcg/bin/gcc -o tocLongFile -I/ldas/ldas-0.0/include -I/ldcg/include tocLongFile.cc  -L/ldas/ldas-0.0/lib -L/ldcg/lib  -lgeneral -lframecpp -lz

readtoc: readtoc.cc
	/ldcg/bin/gcc -o readtoc -I/ldas/ldas-0.0/include -I/ldcg/include readtoc.cc  -L/ldas/ldas-0.0/lib -L/ldcg/lib  -lgeneral -lframecpp -lz

maketoc: maketoc.cc
	/ldcg/bin/gcc -o maketoc -I/ldas/ldas-0.0/include -I/ldcg/include maketoc.cc  -L/ldas/ldas-0.0/lib -L/ldcg/lib  -lgeneral -lframecpp -lz



clean:
	/bin/rm -f frLongFile readframe tocLongFile readtoc maketoc
