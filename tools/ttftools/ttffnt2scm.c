/*
LambdaNative - a cross-platform Scheme framework
Copyright (c) 2009-2013, University of British Columbia
All rights reserved.

Redistribution and use in source and binary forms, with or
without modification, are permitted provided that the
following conditions are met:

* Redistributions of source code must retain the above
copyright notice, this list of conditions and the following
disclaimer.

* Redistributions in binary form must reproduce the above
copyright notice, this list of conditions and the following
disclaimer in the documentation and/or other materials
provided with the distribution.

* Neither the name of the University of British Columbia nor
the names of its contributors may be used to endorse or
promote products derived from this software without specific
prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
// tool to generate texture mapped font from truetype

#include <math.h>
#include <wchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

#include "vec234.h"
#include "vector.h"
#include "texture-atlas.h"
#include "texture-font.h"

// plain ascii
const wchar_t *ascii7_set = L" !\"#$%&'()*+,-./0123456789:;<=>?"
                           L"@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"
                           L"`abcdefghijklmnopqrstuvwxyz{|}~";
// extended ascii
const wchar_t *ascii8_set = L" !\"#$%&'()*+,-./0123456789:;<=>?"
                           L"@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"
                           L"`abcdefghijklmnopqrstuvwxyz{|}~"
                           L"ŠŒŽšžŸœÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÑÒÓÔÕÖØÙÚÛÜÝß"
                           L"àáâãäåæçèéêëìíîïñòóôõöøùúûüýÿ";

int use_extended=0;

void printscmatlas(char *name, texture_atlas_t *a)
{
  int i;
  int w = a->width; 
  int h = a->height;
  int depth = a->depth;
  unsigned char *data = a->data;
  printf("(define %s.raw (glCoreTextureCreate %i %i '#u8(",name, w ,h );
  if (depth==4) {
    for (i=0;i<4*w*h;i++) { printf("%i",(int)data[i]); if (i<4*w*h-1) printf(" "); }
  }
  if (depth==3) {
    for (i=0;i<3*w*h;i++) { printf("%i",(int)data[i]); if (i<3*w*h-1) printf(" "); } 
  } 
  if (depth==1) {
    for (i=0;i<w*h;i++) { printf("%i",(int)data[i]); if (i<w*h-1) printf(" "); } 
  }
  printf(") GL_LINEAR GL_REPEAT))\n");
//  printf(") GL_NEAREST GL_REPEAT))\n");
//  printf("(define %s.img (list %i %i %s.raw 0. 0. 1. 1.))\n", name,w,h,name);
}

void printscmfont(char *tag, texture_font_t *font, int w, int h)
{
  int i, glyph_count = font->glyphs->size;

// special 0 character encodes the total font height
  printf("(list 0 (list 0 %i %s.raw 0. 0. 0. 0.) 0 0 0)\n", (int)ceil(font->ascender-font->descender),tag);

  for( i=0; i < glyph_count; ++i ) {
    texture_glyph_t *glyph = *(texture_glyph_t **) vector_get( font->glyphs, i );
    int c = (int)glyph->charcode;
    if (c>0) {
      printf("(list %i ", (int)glyph->charcode );
      printf("(list %i %i %s.raw ", (int)glyph->width, (int)glyph->height, tag);
//    printf("%f %f %f %f)", glyph->s0, glyph->t1, glyph->s1, glyph->t0 );
      printf("%f %f %f %f)", glyph->s0, glyph->t1+.5/(double)h, glyph->s1+.5/(double)w, glyph->t0 );
      printf(" %i %f %i", (int)glyph->offset_x, glyph->advance_x, (int)glyph->offset_y);
      printf(")\n");
    }
  }
}

size_t tryatlas(size_t w, size_t h, char *fname, int *pointlist,char *tag)
{
  int ptidx=0;
  size_t i, missed=0;
  static texture_atlas_t * atlas = 0;
  if (tag&&atlas) printscmatlas(tag,atlas);
  if (atlas) { texture_atlas_delete(atlas); }
  atlas=texture_atlas_new( w, h, 1 );
 
  while (1) {
    char buf[128];
    i = pointlist[ptidx++];
    if (i==0) break;
    texture_font_t * font = texture_font_new( atlas, fname, i );
    if (tag) {
      sprintf(buf,"%s_%i.fnt", tag, (int)i);
      printf("(define %s (list\n", buf);
      sprintf(buf,"%s.raw", tag);
    }
    missed += texture_font_load_glyphs( font, (use_extended?ascii8_set:ascii7_set));
    if (tag) printscmfont(tag,font,w,h);
    texture_font_delete( font );
    if (tag) printf("))\n");
  }
  return missed;
}

void makeatlas(char *fname, int *pointlist, char *tag)
{
  size_t sizes[]={64,128,256,512,1024};
  int i,j;
  for (j=0;j<5;j++) {
    for (i=0;i<5;i++) {
     if (tryatlas(sizes[i],sizes[j],fname,pointlist,0)==0) goto success;
    }
  }
  fprintf(stderr, "FATAL: Maximum texture size exceeded\n");
  return;
success:
  tryatlas(sizes[i],sizes[j],fname,pointlist,tag);
}

void usage(void)
{
  printf("Usage: ttffnt2scm <ttf file> <ascii bit depth>  <comma separated point sizes> <font tag>\n");
  printf("Generates texture font data on standard out\n");
  exit(0);
} 

int main( int argc, char **argv )
{
  char *s1,*s2;
  int n=0,pointlist[]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  if (argc!=5) usage();
  if (atoi(argv[2])==8) use_extended=1;
  for( s2 = argv[3]; s2; ) {
    while( *s2 == ' ' || *s2 == '\t' ) s2++;
    s1 = strsep( &s2, "," );
    if( *s1 ) {
      int val;
      char ch;
      int ret = sscanf( s1, " %i %c", &val, &ch );
      if( ret == 1 ) { pointlist[n++]=val; }
    }
  }
  makeatlas(argv[1],pointlist,argv[4]);
  return 0;
}

// eof
