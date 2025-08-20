#include <ntifs.h>

extern "C" {
	NTKERNELAPI NTSTATUS IoCreateDriver(PUNICODE_STRING DriverName, PDRIVER_INITIALIZE InitializationFunction);
	NTKERNELAPI NTSTATUS MmCopyVirtualMemory(PEPROCESS SourceProcess,
		PVOID SourceAddress, 
		PEPROCESS TargetProcess, 
		PVOID TargetAddress,
		SIZE_T BufferSize,
		KPROCESSOR_MODE PreviousMode,
		PSIZE_T ReturnSize
	);
}

void debug_prinnt(PCSTR msg) {
	KdPrintEx((
		0,
		0,
		"Centipede: %s\n",
		msg
	));
}

namespace driver {
	namespace codes {
		constexpr ULONG attach = CTL_CODE(FILE_DEVICE_UNKNOWN, 0X696, METHOD_BUFFERED, FILE_ANY_ACCESS);
		constexpr ULONG read = CTL_CODE(FILE_DEVICE_UNKNOWN, 0X697, METHOD_BUFFERED, FILE_ANY_ACCESS);
		constexpr ULONG write = CTL_CODE(FILE_DEVICE_UNKNOWN, 0X698, METHOD_BUFFERED, FILE_ANY_ACCESS);
	}
	struct Request {
		HANDLE process_id;
		PVOID target;
		PVOID buffer;

		SIZE_T size;
		SIZE_T return_Size;
	};

	NTSTATUS create(PDEVICE_OBJECT device_object, PIRP irp) {
		UNREFERENCED_PARAMETER(device_object);

		debug_prinnt("[+] Device opened");
		
		irp->IoStatus.Status = STATUS_SUCCESS;
		irp->IoStatus.Information = 0;
		
		IoCompleteRequest(irp, IO_NO_INCREMENT);

		return STATUS_SUCCESS;
	}

	NTSTATUS close(PDEVICE_OBJECT device_object, PIRP irp) {
		UNREFERENCED_PARAMETER(device_object);

		debug_prinnt("[+] Device closed");
		
		irp->IoStatus.Status = STATUS_SUCCESS;
		irp->IoStatus.Information = 0;
		
		IoCompleteRequest(irp, IO_NO_INCREMENT);

		return STATUS_SUCCESS;
	}

	// Note : Todo
	NTSTATUS device_control(PDEVICE_OBJECT device_object, PIRP irp) {
		UNREFERENCED_PARAMETER(device_object);

		debug_prinnt("[+ centipede +] device control called\n");

		NTSTATUS status = STATUS_UNSUCCESSFUL;

		PIO_STACK_LOCATION stack_irp = IoGetCurrentIrpStackLocation(irp);

		auto request = reinterpret_cast<Request*>(irp->AssociatedIrp.SystemBuffer);

		if (stack_irp == nullptr || request == nullptr) {
			IoCompleteRequest(irp, IO_NO_INCREMENT);
			return status;
		}

		static PEPROCESS target_process = nullptr;

		const ULONG control_code = stack_irp->Parameters.DeviceIoControl.IoControlCode;

		switch (control_code) {

			case codes::attach:
				debug_prinnt("[+] Attach request received");
				// If we already have a process attached, dereference it first
				if (target_process != nullptr) {
					ObDereferenceObject(target_process);
					target_process = nullptr;
				}
				status = PsLookupProcessByProcessId(request->process_id, &target_process);
				if (status == STATUS_SUCCESS) {
					debug_prinnt("[+] Process attached successfully");
				} else {
					debug_prinnt("[-] Failed to attach to process");
				}
				break;
			case codes::read:
				debug_prinnt("[+] Read request received");
				if (target_process != nullptr) {
					status = MmCopyVirtualMemory(
						target_process,
						request->target,
						PsGetCurrentProcess(),
						request->buffer,
						request->size,
						KernelMode,
						&request->return_Size
					);
					if (status == STATUS_SUCCESS) {
						debug_prinnt("[+] Read operation successful");
					} else {
						debug_prinnt("[-] Read operation failed");
					}
				} else {
					debug_prinnt("[-] No target process attached");
					status = STATUS_UNSUCCESSFUL;
				}

				break;

			case codes::write:
				debug_prinnt("[+] Write request received");
				if (target_process != nullptr) {
					status = MmCopyVirtualMemory(
						PsGetCurrentProcess(),
						request->buffer,
						target_process,
						request->target,
						request->size,
						KernelMode,
						&request->return_Size
					);
					if (status == STATUS_SUCCESS) {
						debug_prinnt("[+] Write operation successful");
					} else {
						debug_prinnt("[-] Write operation failed");
					}
				} else {
					debug_prinnt("[-] No target process attached");
					status = STATUS_UNSUCCESSFUL;
				}

				break;

			default:
				break;

		}

		irp->IoStatus.Status = status;
		irp->IoStatus.Information = sizeof(Request);

		IoCompleteRequest(irp, IO_NO_INCREMENT);

		return irp->IoStatus.Status;
	}
}

NTSTATUS driver_main(PDRIVER_OBJECT driver_object, PUNICODE_STRING registry_path) {
	UNREFERENCED_PARAMETER(registry_path);

	// create device_name string
	UNICODE_STRING device_name = {};
	RtlInitUnicodeString(&device_name, L"\\Device\\centipede");

	 // create driver ojbect
	PDEVICE_OBJECT device_object = nullptr;
	NTSTATUS status = IoCreateDevice(driver_object, 0, &device_name, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &device_object);

	// handle if device wasnt created
	if (status != STATUS_SUCCESS) {
		debug_prinnt("[-] Failed To Create Driver Device\n");
		return status;
	}

	debug_prinnt("[+] Driver Device Created Succesfully!");

	UNICODE_STRING symbolic_link = {};
	RtlInitUnicodeString(&symbolic_link, L"\\DosDevices\\centipede");

	// Delete existing symbolic link if it exists
	IoDeleteSymbolicLink(&symbolic_link);
	
	status = IoCreateSymbolicLink(&symbolic_link, &device_name);

	if (status != STATUS_SUCCESS) {
		debug_prinnt("[-] Failed To Create Driver Symbolic Link\n");
		return status;
	}


	debug_prinnt("[+] Driver Symbolic Link Created Succesfully!");


	SetFlag(device_object->Flags, DO_BUFFERED_IO);

	driver_object->MajorFunction[IRP_MJ_CREATE] = driver::create;
	driver_object->MajorFunction[IRP_MJ_CLOSE] = driver::close;
	driver_object->MajorFunction[IRP_MJ_DEVICE_CONTROL] = driver::device_control;

	ClearFlag(device_object->Flags, DO_DEVICE_INITIALIZING);

	// Set up driver unload routine
	driver_object->DriverUnload = [](PDRIVER_OBJECT driver_object) {
		UNICODE_STRING symbolic_link = {};
		RtlInitUnicodeString(&symbolic_link, L"\\DosDevices\\centipede");
		IoDeleteSymbolicLink(&symbolic_link);
		
		if (driver_object->DeviceObject != nullptr) {
			IoDeleteDevice(driver_object->DeviceObject);
		}
		
		debug_prinnt("[+] Driver unloaded successfully");
	};

	debug_prinnt("[+] Driver Initialized Succesfully!");

	return status;
}

NTSTATUS DriverEntry() {
	debug_prinnt("[+] centepede driver loaded");

	// initialize empty driver name
	UNICODE_STRING driver_name = {};

	// fill driver_name with unicode
	RtlInitUnicodeString(&driver_name, L"\\Driver\\centipede");



	return IoCreateDriver(&driver_name,&driver_main);
}