#include <common.h>
#include <s3c2410.h>

#define BUSY            1

#define NAND_SECTOR_SIZE_LP    2048
#define NAND_BLOCK_MASK_LP     (NAND_SECTOR_SIZE_LP - 1)

void nand_init_ll(void);
void nand_read_ll_lp(unsigned char *buf, unsigned long start_addr, int size);

static void s3c2440_nand_reset(void);
static void s3c2440_wait_idle(void);
static void s3c2440_nand_select_chip(void);
static void s3c2440_nand_deselect_chip(void);
static void s3c2440_write_cmd(int cmd);
static void s3c2440_write_addr_lp(unsigned int addr);
static unsigned char s3c2440_read_data(void);


static void s3c2440_nand_reset(void)
{
    s3c2440_nand_select_chip();
    s3c2440_write_cmd(0xff);  // reset command
    s3c2440_wait_idle();
    s3c2440_nand_deselect_chip();
}

static void s3c2440_wait_idle(void)
{
    int i;
	S3C2440_NAND * s3c2440nand = (S3C2440_NAND *)0x4e000000;
    volatile unsigned char *p = (volatile unsigned char *)&s3c2440nand->NFSTAT;

    while(!(*p & BUSY))
        for(i=0; i<10; i++);
}

static void s3c2440_nand_select_chip(void)
{
    int i;
	S3C2440_NAND * s3c2440nand = (S3C2440_NAND *)0x4e000000;

    s3c2440nand->NFCONT &= ~(1<<1);
    for(i=0; i<10; i++);    
}

static void s3c2440_nand_deselect_chip(void)
{
	S3C2440_NAND * s3c2440nand = (S3C2440_NAND *)0x4e000000;

    s3c2440nand->NFCONT |= (1<<1);
}

static void s3c2440_write_cmd(int cmd)
{
	S3C2440_NAND * s3c2440nand = (S3C2440_NAND *)0x4e000000;

    volatile unsigned char *p = (volatile unsigned char *)&s3c2440nand->NFCMD;
    *p = cmd;
}
#if 0
static void s3c2440_write_addr(unsigned int addr)
{
    int i;
	S3C2440_NAND * s3c2440nand = (S3C2440_NAND *)0x4e000000;
    volatile unsigned char *p = (volatile unsigned char *)&s3c2440nand->NFADDR;
    
    *p = addr & 0xff;
    for(i=0; i<10; i++);
    *p = (addr >> 9) & 0xff;
    for(i=0; i<10; i++);
    *p = (addr >> 17) & 0xff;
    for(i=0; i<10; i++);
    *p = (addr >> 25) & 0xff;
    for(i=0; i<10; i++);
}
#endif

static void s3c2440_write_addr_lp(unsigned int addr)
{
    int i;
	S3C2440_NAND * s3c2440nand = (S3C2440_NAND *)0x4e000000;
    volatile unsigned char *p = (volatile unsigned char *)&s3c2440nand->NFADDR;
	int col, page;

	col = addr & NAND_BLOCK_MASK_LP;
	page = addr / NAND_SECTOR_SIZE_LP;
	
    *p = col & 0xff;			/* Column Address A0~A7 */
    for(i=0; i<10; i++);		
    *p = (col >> 8) & 0x0f;		/* Column Address A8~A11 */
    for(i=0; i<10; i++);
    *p = page & 0xff;			/* Row Address A12~A19 */
    for(i=0; i<10; i++);
    *p = (page >> 8) & 0xff;	/* Row Address A20~A27 */
    for(i=0; i<10; i++);
    *p = (page >> 16) & 0x03;	/* Row Address A28~A29 */
    for(i=0; i<10; i++);
}

static unsigned char s3c2440_read_data(void)
{
	S3C2440_NAND * s3c2440nand = (S3C2440_NAND *)0x4e000000;
    volatile unsigned char *p = (volatile unsigned char *)&s3c2440nand->NFDATA;
    return *p;
}


void nand_init_ll(void)
{
	S3C2440_NAND * s3c2440nand = (S3C2440_NAND *)0x4e000000;

#define TACLS   0
#define TWRPH0  3
#define TWRPH1  0

        s3c2440nand->NFCONF = (TACLS<<12)|(TWRPH0<<8)|(TWRPH1<<4);
        s3c2440nand->NFCONT = (1<<4)|(1<<1)|(1<<0);

	s3c2440_nand_reset();
}


void nand_read_ll_lp(unsigned char *buf, unsigned long start_addr, int size)
{
    int i, j;
    
    if ((start_addr & NAND_BLOCK_MASK_LP) || (size & NAND_BLOCK_MASK_LP)) {
        return ;  
    }

    s3c2440_nand_select_chip();

    for(i=start_addr; i < (start_addr + size);) {
      s3c2440_write_cmd(0);

      /* Write Address */
      s3c2440_write_addr_lp(i);
	  s3c2440_write_cmd(0x30);
      s3c2440_wait_idle();

      for(j=0; j < NAND_SECTOR_SIZE_LP; j++, i++) {
          *buf = s3c2440_read_data();
          buf++;
      }
    }

    s3c2440_nand_deselect_chip();
    
    return ;
}

int bBootFrmNORFlash(void)
{
    volatile unsigned int *pdw = (volatile unsigned int *)0;
    unsigned int dwVal;
    
    dwVal = *pdw;       
    *pdw = 0x12345678;
    if (*pdw != 0x12345678)
    {
        return 1;
    }
    else
    {
        *pdw = dwVal;
        return 0;
    }
}

int CopyCode2Ram(unsigned long start_addr, unsigned char *buf, int size)
{
    unsigned int *pdwDest;
    unsigned int *pdwSrc;
    int i;

    if (bBootFrmNORFlash())
    {
        pdwDest = (unsigned int *)buf;
        pdwSrc  = (unsigned int *)start_addr;
        for (i = 0; i < size / 4; i++)
        {
            pdwDest[i] = pdwSrc[i];
        }
        return 0;
    }
    else
    {
		nand_init_ll();
        nand_read_ll_lp(buf, start_addr, (size + NAND_BLOCK_MASK_LP)&~(NAND_BLOCK_MASK_LP));
		return 0;
    }
}

static inline void delay (unsigned long loops)
{
    __asm__ volatile ("1:\n"
      "subs %0, %1, #1\n"
      "bne 1b":"=r" (loops):"0" (loops));
}

/* S3C2440: Mpll = (2*m * Fin) / (p * 2^s), UPLL = (m * Fin) / (p * 2^s)
 * m = M (the value for divider M)+ 8, p = P (the value for divider P) + 2
 */
#define S3C2440_MPLL_400MHZ     ((0x5c<<12)|(0x01<<4)|(0x01))
#define S3C2440_MPLL_200MHZ     ((0x5c<<12)|(0x01<<4)|(0x02))
#define S3C2440_MPLL_100MHZ     ((0x5c<<12)|(0x01<<4)|(0x03))
#define S3C2440_UPLL_96MHZ      ((0x38<<12)|(0x02<<4)|(0x01))
#define S3C2440_UPLL_48MHZ      ((0x38<<12)|(0x02<<4)|(0x02))
#define S3C2440_CLKDIV          (0x05) // | (1<<3))    /* FCLK:HCLK:PCLK = 1:4:8, UCLK = UPLL/2 */
#define S3C2440_CLKDIV188       0x04    /* FCLK:HCLK:PCLK = 1:8:8 */
#define S3C2440_CAMDIVN188      ((0<<8)|(1<<9)) /* FCLK:HCLK:PCLK = 1:8:8 */

/* Fin = 16.9344MHz */
#define S3C2440_MPLL_399MHz_Fin16MHz	((0x6e<<12)|(0x03<<4)|(0x01))
#define S3C2440_UPLL_48MHZ_Fin16MHz     ((60<<12)|(4<<4)|(2))

void clock_init(void)
{
	S3C24X0_CLOCK_POWER *clk_power = (S3C24X0_CLOCK_POWER *)0x4C000000;

        /* FCLK:HCLK:PCLK = 1:4:8 */
        clk_power->CLKDIVN = S3C2440_CLKDIV;

        /* change to asynchronous bus mod */
        __asm__(    "mrc    p15, 0, r1, c1, c0, 0\n"    /* read ctrl register   */  
                    "orr    r1, r1, #0xc0000000\n"      /* Asynchronous         */  
                    "mcr    p15, 0, r1, c1, c0, 0\n"    /* write ctrl register  */  
                    :::"r1"
                    );

        /* to reduce PLL lock time, adjust the LOCKTIME register */
        clk_power->LOCKTIME = 0xFFFFFFFF;

        /* configure UPLL */
        clk_power->UPLLCON = S3C2440_UPLL_48MHZ;

        /* some delay between MPLL and UPLL */
        delay (4000);

        /* configure MPLL */
        clk_power->MPLLCON = S3C2440_MPLL_400MHZ;

        /* some delay between MPLL and UPLL */
        delay (8000);
}

