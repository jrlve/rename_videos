build:
	gcc bulk_rename.c -o  bulk_rename

debug:
	gcc -g bulk_rename.c -o bulk_rename

clean:
	rm bulk_rename
