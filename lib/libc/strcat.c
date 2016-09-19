char * strcat(char *p1, char* p2)
{
    int i ;

    for ( i=0;  *p1 != '\0'; ++i, ++p1 ) ; 
    for( ; *p2 != '\0'; i++ )
        p1[i] = *p2++ ;

    p1[i] = '\0' ;
    return(p1) ;
}
