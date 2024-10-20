#include <stdio.h>
#include "pci_c_header.h"
#include <sys/io.h>
#include <stdbool.h>

const int BusIter = 65536;
const int DevIter = 2048;
const int FuncIter = 256;
const unsigned int startAddr = 2147483648;
#define CONTOROLER_NOT_EXIT 0xFFFF;

void PrintControlerInfo(int BusNumb,int DevNumb, int FuncNumb);

int main(){

    if(iopl(3)) 
	{
		printf("I/O Privilege level change error: \nTry running under ROOT user\n");
		return 1;
	} 
	
    for (int i = 0; i < 256;i++){
        for (int j = 0; j < 32;j++){
            for (int k = 0; k < 8; k++){
                
                PrintControlerInfo(i,j,k);

            }
        }
    }

    return 0;
}
const char *PrintVendorName(int VenId){
    for (int i = 0; i < PCI_VENTABLE_LEN;i++){
        if ( VenId == PciVenTable[i].VenId){
            printf("Vendor name: %s ;\n",PciVenTable[i].VenFull);
        }

    }
}

void PrintDeviceName(int DevId,  int VenId){
    for (int i = 0; i < PCI_DEVTABLE_LEN;i++){
        if ( (DevId == PciDevTable[i].DevId) && (VenId == PciDevTable[i].VenId)){
            //printf(PciDevTable[i].Chip);
            printf("Chip name: %s ;\n",PciDevTable[i].ChipDesc);
        }

    }
}
bool IsBridge(unsigned int addr){
    
    outl(addr+0x0C,0x0CF8);
    unsigned int Temp = inl(0x0CFC);
    Temp = Temp >> 16;
    return Temp & 0x01 == 0x01 ? true : false;
}

void PrintBusNumbers(unsigned int addr){
    
    outl(addr+0x18,0x0CF8);
    unsigned int Temp = inl(0x0CFC);

    printf("Primary Bus Number: %04X;\n",Temp & 0x000000FF);
    printf("Secondary Bus Number: %04X;\n",(Temp >> 8)& 0x000000FF);
    printf("Subordinate Bus Number: %04X;\n",(Temp >> 16)& 0x000000FF) ;

    
}

void PrintIORegisters(unsigned int addr){
    
    outl(addr+0x1C,0x0CF8);
    unsigned int Temp = inl(0x0CFC);

    printf("I/O Base: %02X;\n",(Temp & 0x000000FF)) ;
    printf("I/O Limit: %02X;\n",((Temp >> 8)& 0x000000FF) );
    
}

void PrintRomInfo(unsigned int addr){
    
    outl(addr+0x30,0x0CF8);
    unsigned int Temp = inl(0x0CFC);

    printf("ROM : %08X;\n",(Temp )) ;
    printf("ROM : %08X;\n",(Temp >> 11 )) ;
}

void PrintControlerInfo(int BusNumb,int DevNumb, int FuncNumb){
    unsigned int addr = (1 << 31) + (BusNumb<< 16) + (DevNumb << 11) + (FuncNumb << 8);
    
    outl(addr,0x0CF8);
    unsigned int DevId = inl(0x0CFC);
    
    if ((DevId >> 16) != 0xFFFF){
        
        unsigned int  VendId = DevId & 0x0000FFFF;
        DevId = DevId >> 16;
        printf("Addr: %08X\n",addr);
        printf("Device Id: %04X;\nVendor Id: %04X;\n",DevId,VendId);
        PrintVendorName(VendId);
        PrintDeviceName(DevId,VendId);
        if (IsBridge(addr)){
            PrintBusNumbers(addr);
            PrintIORegisters(addr);
        } else {
            PrintRomInfo(addr);
        }
        printf("\n");
        //Rom memory
        //
    } 
  
}

