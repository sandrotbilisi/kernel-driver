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
		DPFLTR_IHVDRIVER_ID,
		DPFLTR_INFO_LEVEL,
		"Centipede: %s\n",
		msg
	));
}

namespace driver {
	namespace codes {
		constexpr ULONG attach = CTL_CODE(FILE_DEVICE_UNKNOWN, 0X696, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
		constexpr ULONG read = CTL_CODE(FILE_DEVICE_UNKNOWN, 0X697, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
		constexpr ULONG write = CTL_CODE(FILE_DEVICE_UNKNOWN, 0X698, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
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

		IoCompleteRequest(irp, IO_NO_INCREMENT);

		return irp->IoStatus.Status;
	}

	NTSTATUS close(PDEVICE_OBJECT device_object, PIRP irp) {
		UNREFERENCED_PARAMETER(device_object);

		IoCompleteRequest(irp, IO_NO_INCREMENT);

		return irp->IoStatus.Status;
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
				status = PsLookupProcessByProcessId(request->process_id, &target_process);
				break;
			case codes::read:
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
				}

				break;

			case codes::write:

				status = MmCopyVirtualMemory(
					PsGetCurrentProcess(),
					request->buffer,
					target_process,
					request->target,
					request->size,
					KernelMode,
					&request->return_Size
				);


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