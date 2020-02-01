
#import <Foundation/Foundation.h>
#import <stdio.h>

void Pravala_NSLog ( const char * format, ...)
{
    char buffer[ 4096 ] = { 0 };

    va_list args;
    va_start ( args, format );
    vsnprintf ( buffer, sizeof ( buffer ), format, args );
    va_end ( args );

    NSLog ( @"%s", buffer );
}
