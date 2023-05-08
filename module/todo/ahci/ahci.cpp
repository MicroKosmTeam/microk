#include "ahci.hpp"

namespace AHCI {
        #define HBA_PORT_DEV_PRESENT 0x3
        #define HBA_PORT_IPM_ACTIVE  0x1
        #define SATA_SIG_ATAPI       0xEB140101
        #define SATA_SIG_ATA         0x00000101
        #define SATA_SIG_SEMB        0xC33C0101
        #define SATA_SIG_PM          0x96690101
        #define HBA_PxCMD_CR         0x8000
        #define HBA_PxCMD_FRE        0x0010
        #define HBA_PxCMD_ST         0x0001
        #define HBA_PxCMD_FR         0x4000

        PortType CheckPortType(HBAPort *port) {
                uint32_t sataStatus = port->sataStatus;

                uint8_t interfacePowerManagment = (sataStatus >> 8) & 0b111;
                uint8_t deviceDetection = sataStatus & 0b111;

                if (deviceDetection != HBA_PORT_DEV_PRESENT) return PortType::None;
                if (interfacePowerManagment != HBA_PORT_IPM_ACTIVE) return PortType::None;

                switch (port->signature) {
                        case SATA_SIG_ATAPI:
                                return PortType::SATAPI;
                        case SATA_SIG_ATA:
                                return PortType::SATA;
                        case SATA_SIG_SEMB:
                                return PortType::SEMB;
                        case SATA_SIG_PM:
                                return PortType::PM;
                        default:
                                return PortType::None;
                }
        }

        void Port::Configure() {
                StopCMD();

                void *newBase = RequestPage();
                hbaPort->commandListBase = (uint32_t)(uint64_t)newBase;
                hbaPort->commandListBaseUpper = (uint32_t)((uint64_t)newBase >> 32);
                Memset((void*)(hbaPort->commandListBase), 0, 1024);

                void *fisBase = RequestPage();
                hbaPort->fisBaseAddress = (uint32_t)(uint64_t)fisBase;
                hbaPort->fisBaseAddressUpper = (uint32_t)((uint64_t)fisBase >> 32);
                Memset(fisBase, 0, 256);

                HBACommandHeader *cmdHeader = (HBACommandHeader*)((uint64_t)hbaPort->commandListBase + ((uint64_t)hbaPort->commandListBaseUpper << 32));

                for (int i = 0; i < 32; i++) {
                        cmdHeader[i].prdtLength = 8;

                        void * cmdTableAddress = RequestPage();
                        uint64_t address = (uint64_t)cmdTableAddress + (i << 8);
                        cmdHeader[i].commandTableBaseAddress = (uint32_t)address;
                        cmdHeader[i].commandTableBaseAddressUpper = (uint32_t)((uint64_t)address >> 32);
                        Memset(cmdTableAddress, 0, 256);
                }

                StartCMD();
        }

        void Port::StartCMD() {
                while(hbaPort->cmdSts & HBA_PxCMD_CR);

                hbaPort->cmdSts |= HBA_PxCMD_FRE;
                hbaPort->cmdSts |= HBA_PxCMD_ST;
        }

        void Port::StopCMD() {
                hbaPort->cmdSts &= ~HBA_PxCMD_ST;
                hbaPort->cmdSts &= ~HBA_PxCMD_FRE;

                while(true) {
                        if(hbaPort->cmdSts & HBA_PxCMD_FR) continue;
                        if(hbaPort->cmdSts & HBA_PxCMD_CR) continue;

                        break;
                }
        }

        bool Port::TransferDMA(bool write, uint64_t sector, uint32_t sectorCount, void* buffer) {
                // Control if busy
                uint64_t spin = 0;
                while ((hbaPort->taskFileData & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin < 1000000){
                        spin++; // Timeout
                }
                if (spin == 1000000) {
                        return false;
                }

                uint32_t sectorLow = (uint32_t)sector;
                uint32_t sectorHigh = (uint32_t)(sector >> 32);
                hbaPort->interruptStatus = (uint32_t)-1; // Clean interrupt line

                HBACommandHeader *cmdHeader = (HBACommandHeader*)hbaPort->commandListBase;
                cmdHeader->commandFISLength = sizeof(FIS_REG_H2D)/sizeof(uint32_t); // Command FIS size
                cmdHeader->write = write ? 1 : 0; // Is this a write
                cmdHeader->prdtLength = 1;

                HBACommandTable *commandTable = (HBACommandTable*)(cmdHeader->commandTableBaseAddress);
                Memset(commandTable, 0, sizeof(HBACommandTable) + (cmdHeader->prdtLength-1)*sizeof(HBAPRDTEntry));

                commandTable->prdtEntry[0].dataBaseAddress = (uint32_t)(uint64_t)buffer;
                commandTable->prdtEntry[0].dataBaseAddressUpper = (uint32_t)((uint64_t)buffer >> 32);
                commandTable->prdtEntry[0].byteCount = (sectorCount<<9)-1; // 512 bytes per sector
                commandTable->prdtEntry[0].interruptOnCompletion = 1;

                FIS_REG_H2D *cmdFIS = (FIS_REG_H2D*)(&commandTable->commandFIS);

                cmdFIS->fisType = FIS_TYPE_REG_H2D;
                cmdFIS->commandControl = 1; //It's a command
                cmdFIS->command = write ? ATA_CMD_WRITE_DMA_EX : ATA_CMD_READ_DMA_EX;

                cmdFIS->lba0 = (uint8_t)sectorLow;
                cmdFIS->lba1 = (uint8_t)(sectorLow >> 8);
                cmdFIS->lba2 = (uint8_t)(sectorLow >> 16);
                cmdFIS->lba3 = (uint8_t)sectorHigh;
                cmdFIS->lba4 = (uint8_t)(sectorHigh >> 8);
                cmdFIS->lba5 = (uint8_t)(sectorHigh >> 16);

                cmdFIS->deviceRegister = 1 << 6; // LBA mode

                cmdFIS->countLow = sectorCount & 0xFF;
                cmdFIS->countHigh = (sectorCount >> 8) & 0xFF;

                hbaPort->commandIssue = 1;

                PrintK("AHCI command sent for sector %d.\n", sector);

                // Wait until done (fix it with interrupts)
                while (true){
                        if((hbaPort->commandIssue == 0)) break;
                        if(hbaPort->interruptStatus & HBA_PxIS_TFES) {
                                // Task file error
                                return false;
                        }
                }

                return true;
        }
        bool Port::Write(uint64_t sector, uint32_t sectorCount, void* buffer) {
                return TransferDMA(true, sector, sectorCount, buffer);
        }

        bool Port::Read(uint64_t sector, uint32_t sectorCount, void* buffer) {
                return TransferDMA(false, sector, sectorCount, buffer);
        }

        AHCIDriver::AHCIDriver(PCI::PCIDeviceHeader *pciBaseAddress) {
                this->PCIBaseAddress = pciBaseAddress;
                PrintK("AHCI instance initialized.\r\n");

                ABAR = (HBAMemory*)((PCI::PCIHeader0*)pciBaseAddress)->BAR5;
                //VMM::MapMemory(ABAR, ABAR);

                ProbePorts();
                PrintK("Ports probed.\r\n");

                for (int i = 0; i < portCount; i++) {
                        Port *port = ports[i];

			if (port == NULL) continue;
			PrintK("Port number %d available.\r\n", i);

			port->Configure();
			port->buffer = (uint8_t*)RequestPage(); //(0x1000/0x1000);
			port->Read(0, 1, port->buffer);

			PrintK("Printing first sector:\r\n");
			for (int i = 0; i < 512; i++) {
				PrintK("%c", port->buffer[i]);
			}

			PrintK("\r\nDone.\r\n");
                }
        }

        AHCIDriver::~AHCIDriver() {

        }

        void AHCIDriver::ProbePorts() {
                uint32_t portsImplemented = ABAR->portsImplemented;
		PrintK("Probing for ports.\r\n");
                for (int i = 0; i < 32; i++) {
                        if (portsImplemented & (1 << i)) {
                                PortType portType = CheckPortType(&ABAR->ports[i]);

                                if (portType == PortType::SATA || portType == PortType::SATAPI) {
                                        ports[portCount] = new Port();
                                        ports[portCount]->portType = portType;
                                        ports[portCount]->hbaPort = &ABAR->ports[i];
                                        ports[portCount]->portNumber = portCount;
                                        portCount++;
                                }

                        }
                }
        }
}
