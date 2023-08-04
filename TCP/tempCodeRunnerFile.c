long file_transfer(char* buffer, int t){
//     int fp = open(buffer, O_RDONLY);
//     if (fp == -1){
//         perror("Error reading file\n");
//         exit(1);
//     }
//     long offset = 0;
//     while (offset < BUFFLEN){
//         ssize_t readnow = readn(fp, buffer+offset, 1);
//         if (readnow == 0){
//             break;
//         }
//         else offset = offset+readnow;
//     }
//     close(fp);
//     printf("File read complete \n");
//     return offset;
// }
