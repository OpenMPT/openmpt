open(I, "mptrack.rc");

while(<I>) {
    if (/\VALUE "FileVersion", "([0-9]*), ([0-9]*), ([0-9]*), ([0-9]*)".*/) {
        printf("%d.%02d.%02d.%02d", $1, $2, $3, $4);
    }
}

close(I);