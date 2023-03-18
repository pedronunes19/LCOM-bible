#include <lcom/lcf.h>
#include "graphics.h"
#include <math.h>

// Mudança do Minix para modo gráfico
int (set_graphic_mode)(uint16_t submode) {
    reg86_t reg86;
    memset(&reg86, 0, sizeof(reg86)); // inicialização da estrutura com o valor 0 em todos os parâmetros
    reg86.intno = 0x10;               // intno é sempre 0x10      
    reg86.ah = 0x4F;                  // parte mais significativa de AX
    reg86.al = 0x02;                  // parte menos significativa de AX. 0x02 no caso de modo gráfico
    // reg86.ax = 0x4F02;             // equivamente às duas últimas instruções
    reg86.bx = submode | BIT(14);     // determinação do submodo com memória linear
    if (sys_int86(&reg86) != 0) {     // se houver algum erro, abortar a função
        printf("Set graphic mode failed\n");
        return 1;
    }
    return 0;
}

// Mudança do Minix para modo de texto
// Implementação interna da função vg_exit() já dada pela LCF
int (set_text_mode)() {
    reg86_t reg86;                       
    memset(&reg86, 0, sizeof(reg86));   // inicialização da estrutura com o valor 0 em todos os parâmetros
    reg86.intno = 0x10;                 // intno é sempre 0x10 
    reg86.ah = 0x00;                    // parte mais significativa de AX 
    reg86.al = 0x03;                    // parte menos significativa de AX. 0x03 no caso de modo texto
    // reg86.ax = 0x0003;               // equivamente às duas últimas instruções
    reg86.bx = 0x0000;                  // não há submodo no modo de texto
    if(sys_int86(&reg86) != 0) {        // se houver algum erro, abortar a função
        printf("Set text mode failed\n");
        return 1;
    }
    return 0;
}

// rever =========================== ->

int (map_vmem)(uint16_t mode){

  memset(&info, 0, sizeof(info));
  if(vbe_get_mode_info(mode,&info)) return 1;

  int r;
  uint8_t* video_mem;
  struct minix_mem_range mr; /* physical memory range */
  unsigned int vram_base = info.PhysBasePtr; /* VRAM’s physical address */
  unsigned int vram_size = (info.XResolution * info.YResolution * info.BitsPerPixel) / 8; /* VRAM’s size, but you can use the frame-buffer size, instead */

  /* Allow memory mapping */
  mr.mr_base = (phys_bytes) vram_base;
  mr.mr_limit = mr.mr_base + vram_size;
  if( OK != (r = sys_privctl(SELF, SYS_PRIV_ADD_MEM, &mr))){
    panic("sys_privctl (ADD_MEM) failed: %d\n", r);
    return 1;
  }

  /* Map memory */
  video_mem = vm_map_phys(SELF, (void *)mr.mr_base, vram_size);
  if(video_mem == MAP_FAILED){
    panic("couldn’t map video memory");
    return 1;
  }

  vAddr_base = video_mem;
  return 0;
}

int (vg_draw_pixel)(uint16_t x, uint16_t y, uint32_t color) {

  if(x >= info.XResolution || y >= info.YResolution) return 1;
  
  unsigned bpp = (info.BitsPerPixel + 7) / 8;

  memcpy(&vAddr_base[(info.XResolution*y + x) * bpp], &color, bpp);

  return 0;
}

int (vg_draw_hline)(uint16_t x, uint16_t y, uint16_t len, uint32_t color){
  for(unsigned i = 0; i < len; i++)
    if (vg_draw_pixel(x+i,y,color)) return 1;

  return 0;
}

int (vg_draw_rectangle)(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color) {
  for(unsigned i = 0; i < height; i++)
    if (vg_draw_hline(x,y+i,width,color) != 0) {
      vg_exit();
      return 1;
    }
  return 0;
}

uint32_t (direct_mode)(uint32_t R, uint32_t G, uint32_t B) {
  return (R << info.RedFieldPosition) | (G << info.GreenFieldPosition) | (B << info.BlueFieldPosition);
}

uint32_t (indexed_mode)(uint16_t col, uint16_t row, uint8_t step, uint32_t first, uint8_t n) {
  return (first + (row * n + col) * step) % (1 << info.BitsPerPixel);
}

uint32_t (Red)(unsigned j, uint8_t step, uint32_t first) {
  return (R(first) + j * step) % (1 << info.RedMaskSize);
}

uint32_t (Green)(unsigned i, uint8_t step, uint32_t first) {
  return (G(first) + i * step) % (1 << info.GreenMaskSize);
}

uint32_t (Blue)(unsigned j, unsigned i, uint8_t step, uint32_t first) {
  return (B(first) + (i + j) * step) % (1 << info.BlueMaskSize);
}

/* Funções auxiliares de cores */

uint32_t (R)(uint32_t first){
  return ((1 << info.RedMaskSize) - 1) & (first >> info.RedFieldPosition);
}

uint32_t (G)(uint32_t first){
  return ((1 << info.GreenMaskSize) - 1) & (first >> info.GreenFieldPosition);
}

uint32_t (B)(uint32_t first){
  return ((1 << info.BlueMaskSize) - 1) & (first >> info.BlueFieldPosition);
}
