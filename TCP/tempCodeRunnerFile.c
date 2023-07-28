    FILE *fp;
    if (mode == AFTER) {fp = fopen(file, "ab+"); printf("1");}
    else if (mode == FIRST) {fp = fopen(file, "wb+");