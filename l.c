#include<stdio.h>
#include <sys/io.h>
#include <string.h>
#include "pci_c_header.h"

void printBin(int num){
    unsigned mask = 1 << 31;
    for (int i = 0;i<32;i++) {
        printf("%d", (num & mask) != 0);
        mask >>= 1;
    }
    printf("\n");
}

unsigned int getPCIData(const unsigned int addr, const unsigned int reg);
void getBussesNums(const unsigned int portData, unsigned int *primBusNum, unsigned int *secBusNum, unsigned int *subBusNum);

#define oldBit 0x80000000 
#define headerType 0x00800000
#define PCI_manage_reg 0xCF8
#define PCI_data_reg 0xCFC

void printName(unsigned int idReg){
    unsigned int devId = (idReg & 0xFFFF0000) >> 16;
    unsigned int venId = (idReg & 0x0000FFFF);
    for (int i = 0;i < PCI_VENTABLE_LEN; i++){
        if (PciVenTable[i].VenId == venId){
            printf("######\nVendor ID: %04x; Vendor short name: %s; Vendor full name: %s.\n", PciVenTable[i].VenId, PciVenTable[i].VenShort, PciVenTable[i].VenFull);
            break;
        }
    }
    for (int i = 0;i < PCI_DEVTABLE_LEN; i++){
        if (PciDevTable[i].DevId == devId && PciDevTable[i].VenId == venId){
            printf("Chip: %s\nChip description: %s-\n-----\n", PciDevTable[i].Chip, PciDevTable[i].ChipDesc);
            return;
        }
    }
}
void printPCIDeviceNumber(unsigned PCIBusNum, unsigned int deviceNum, unsigned int funcNum){
    printf("--\nPCI bus number: %02x\nDevice number: %02x\nFunction number: %02x\n--\n", PCIBusNum >> 16, deviceNum >> 11, funcNum >> 8);
}
int printClassCode(unsigned int classCode){

    int base = (classCode & 0x00ff0000) >> 16;
    int sub = (classCode & 0x0000ff00) >> 8;
    int spec = (classCode & 0x000000ff);
    
    printf("Class code: %06x\n", classCode);
    for (int i = 0; i < PCI_CLASSCODETABLE_LEN;i ++){
        if (PciClassCodeTable[i].BaseClass == base &&
            PciClassCodeTable[i].SubClass == sub &&
            PciClassCodeTable[i].ProgIf == spec){

                printf("Base class: %s\n", PciClassCodeTable[i].BaseDesc);
                printf("Subclass: %s\n", PciClassCodeTable[i].SubDesc);
                printf("Specific register: %s\n", PciClassCodeTable[i].ProgDesc);

                return 0;
        }
    }
    
    return 1;
}
void printBridgeInfo(unsigned int portData, unsigned int baseAddr, unsigned PCIBusNum, unsigned int deviceNum, unsigned int funcNum){
    unsigned int primBusNum, secBusNum, subBusNum;

    getBussesNums(portData, &primBusNum, &secBusNum, &subBusNum);
    printf("Is bridge.\n");
    printPCIDeviceNumber(PCIBusNum, deviceNum, funcNum);
    printf("Primary buss number: %x\nSecondary buss number: %x\nSuboradinate buss number: %x\n--------\n", primBusNum, secBusNum, subBusNum);        
}
void printDeviceInfo(unsigned int intPin, unsigned int classCode, unsigned PCIBusNum, unsigned int deviceNum, unsigned int funcNum){
    printf("Is device.\n");
    printPCIDeviceNumber(PCIBusNum, deviceNum, funcNum);
    printf("Interrupt pin: %02x\n", intPin);
    
    if (printClassCode(classCode)){
        printf("!There is no any information about device's class.\n");
    }
    printf("*********\n");
}

int exists(unsigned int portData){
    return ~portData;
}
int isBridge(unsigned int portData){
    return (portData & headerType);
}

unsigned int getBaseAddr(const unsigned int PCIBusNum, const unsigned int deviceNum, const unsigned int funcNum){
    return oldBit | PCIBusNum | deviceNum | funcNum;
}
void getBussesNums(const unsigned int portData, unsigned int *primBusNum, unsigned int *secBusNum, unsigned int *subBusNum){
    *subBusNum = (portData & 0x00FF0000) >> 16;
    *secBusNum = (portData & 0x0000FF00) >> 8;
    *primBusNum = (portData & 0x000000FF);
}
unsigned int getPCIData(const unsigned int addr, const unsigned int reg){
    outl(addr + reg, PCI_manage_reg);
    return inl(PCI_data_reg);
}
unsigned int getIntPin(unsigned int portData){
    return (portData >> 8) && 0xFF;
}
unsigned int getClassCode(unsigned int portData){
    return (portData >> 8);
}

unsigned int decodeIntPort(unsigned int baseAddr){
    #define intReg 0x3c
    unsigned int portData = getPCIData(baseAddr, intReg);
    unsigned int intPin = getIntPin(portData);
    return intPin;
}
unsigned int decodeClassCode(unsigned int baseAddr){
    #define classReg 0x08
    unsigned int portData = getPCIData(baseAddr, classReg);
    return getClassCode(portData);
}

void checkFunctions(const int funcCount, const int funcStep, 
                    const int PCIBusNum, const int deviceNum){

    const int busReg = 0x18, headReg = 0x0c;
    unsigned int portData, baseAddr = 0, intPin, classCode;

    for (int funcNum = 0;funcNum < funcCount; funcNum += funcStep){

        baseAddr = getBaseAddr(PCIBusNum, deviceNum, funcNum);
        portData = getPCIData(baseAddr, 0);

        if (exists(portData)){
            printName(portData);
            portData = getPCIData(baseAddr, headReg);
            
            if (isBridge(portData)){

                portData = getPCIData(baseAddr, busReg);
                printBridgeInfo(portData, baseAddr, PCIBusNum, deviceNum, funcNum);
            }
             else {

                intPin = decodeIntPort(baseAddr);
                classCode = decodeClassCode(baseAddr);
                
                printDeviceInfo(intPin, classCode, PCIBusNum, deviceNum, funcNum);
            
            }
        }
    }
}
void checkDevices(const int deviceCount, const int deviceStep, const int PCIBusNum){
    for (int deviceNum = 0;deviceNum < deviceCount; deviceNum += deviceStep){
        checkFunctions(1 << 11, 1 << 8, PCIBusNum, deviceNum);
        //break;
    }
}
void checkPCIBuses(const int PCIBusCount, const int PCIBusStep){
    for (int PCIBusNum = 0; PCIBusNum < PCIBusCount; PCIBusNum += PCIBusStep){
        checkDevices(1 << 16, 1 << 11, PCIBusNum);
        //break;
    }
}
void app (){
    checkPCIBuses(1 << 24, 1 << 16);
}

int main(){
    int errno;
    if (errno = iopl(3)){
        printf("I/O privelege level change error: %s\nTry running under ROOT user\n", (char *)strerror(errno));
        return 1;
    } else {
        app();
    }
    
    return 0;
}