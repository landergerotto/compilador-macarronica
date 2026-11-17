BEGIN { FS="," }
NR>1 { count++; labels[count]=$1 " N=" $2 " " $3 " t=" $4; values[count]=$5*1000; if(values[count]>maximum) maximum=values[count] }
END {
    height=100+count*28;
    printf "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"1100\" height=\"%d\" viewBox=\"0 0 1100 %d\"><rect width=\"100%%\" height=\"100%%\" fill=\"white\"/><g font-family=\"sans-serif\" font-size=\"13\" fill=\"#172337\">", height, height;
    print "<text x=\"20\" y=\"25\" font-size=\"18\">Macaronica: mediana por kernel (ms, menor e melhor)</text>";
    if (!count) print "<text x=\"20\" y=\"60\">Nenhuma medicao disponivel; consulte status.csv.</text>";
    for(i=1;i<=count;i++) {
        y=55+(i-1)*28;
        width=maximum>0 ? values[i]/maximum*600 : 0;
        printf "<text x=\"20\" y=\"%d\">%s</text><rect x=\"320\" y=\"%d\" width=\"%.3f\" height=\"18\" fill=\"#3b73b9\"/><text x=\"%.3f\" y=\"%d\">%.6f ms</text>",y+13,labels[i],y,width,930,y+13,values[i];
    }
    print "</g></svg>";
}
