#ifndef FRLG_NATIVE_MULTIBOOT_H
#define FRLG_NATIVE_MULTIBOOT_H

typedef enum FrlgNativeMultibootStatus {
    FRLG_NATIVE_MULTIBOOT_OK = 0,
    FRLG_NATIVE_MULTIBOOT_SERIAL_UNSUPPORTED,
    FRLG_NATIVE_MULTIBOOT_EXECUTE_UNSUPPORTED,
    FRLG_NATIVE_MULTIBOOT_TRANSFER_UNSUPPORTED
} FrlgNativeMultibootStatus;

/* The first unsupported operation remains visible until the next Init. */
FrlgNativeMultibootStatus frlg_native_multiboot_status(void);

#endif
