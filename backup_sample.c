// Copyright Cloudbase Solutions SRL 2021

/*
 * To run this sample:
    VEEAMSNAP_ROOT="/path/to/the/veeamsnap/repo/you/built"
    gcc -Wall -I${VEEAMSNAP_ROOT} -o backup_sample backup_sample.c
    sudo ./backup_sample.exe
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "source/veeamsnap_ioctl.h"


#define VEEAM_SNAP_DEVICE_PATH "/dev/veeamsnap"


/*
void print_ioctl_codes() {
	struct ioctl_call {
		unsigned long long command;
		char* name;
	};
	struct ioctl_call ioctl_calls[] = {
                // {SUCCESS, "SUCCESS"},
                {MAX_TRACKING_DEVICE_COUNT, "MAX_TRACKING_DEVICE_COUNT"},
                {VEEAM_SNAP, "VEEAM_SNAP"},
                {VEEAMSNAP_COMPATIBILITY_SNAPSTORE, "VEEAMSNAP_COMPATIBILITY_SNAPSTORE"},
                {VEEAMSNAP_COMPATIBILITY_BTRFS, "VEEAMSNAP_COMPATIBILITY_BTRFS"},
                {VEEAMSNAP_COMPATIBILITY_MULTIDEV, "VEEAMSNAP_COMPATIBILITY_MULTIDEV"},
                {IOCTL_COMPATIBILITY_FLAGS, "IOCTL_COMPATIBILITY_FLAGS"},
                {IOCTL_GETVERSION, "IOCTL_GETVERSION"},
                {IOCTL_TRACKING_ADD, "IOCTL_TRACKING_ADD"},
                {IOCTL_TRACKING_REMOVE, "IOCTL_TRACKING_REMOVE"},
                {IOCTL_TRACKING_COLLECT, "IOCTL_TRACKING_COLLECT"},
                {IOCTL_TRACKING_BLOCK_SIZE, "IOCTL_TRACKING_BLOCK_SIZE"},
                {IOCTL_TRACKING_READ_CBT_BITMAP, "IOCTL_TRACKING_READ_CBT_BITMAP"},
                {IOCTL_TRACKING_MARK_DIRTY_BLOCKS, "IOCTL_TRACKING_MARK_DIRTY_BLOCKS"},
                {IOCTL_SNAPSHOT_CREATE, "IOCTL_SNAPSHOT_CREATE"},
                {IOCTL_SNAPSHOT_DESTROY, "IOCTL_SNAPSHOT_DESTROY"},
                {IOCTL_SNAPSHOT_ERRNO, "IOCTL_SNAPSHOT_ERRNO"},
                {IOCTL_SNAPSTORE_CREATE, "IOCTL_SNAPSTORE_CREATE"},
                {IOCTL_SNAPSTORE_FILE, "IOCTL_SNAPSTORE_FILE"},
                {IOCTL_SNAPSTORE_MEMORY, "IOCTL_SNAPSTORE_MEMORY"},
                {IOCTL_SNAPSTORE_CLEANUP, "IOCTL_SNAPSTORE_CLEANUP"},
                {IOCTL_SNAPSTORE_FILE_MULTIDEV, "IOCTL_SNAPSTORE_FILE_MULTIDEV"},
                {IOCTL_COLLECT_SNAPSHOT_IMAGES, "IOCTL_COLLECT_SNAPSHOT_IMAGES"},
                {IOCTL_COLLECT_SNAPSHOTDATA_LOCATION_START, "IOCTL_COLLECT_SNAPSHOTDATA_LOCATION_START"},
                {IOCTL_COLLECT_SNAPSHOTDATA_LOCATION_GET, "IOCTL_COLLECT_SNAPSHOTDATA_LOCATION_GET"},
                {IOCTL_COLLECT_SNAPSHOTDATA_LOCATION_COMPLETE, "IOCTL_COLLECT_SNAPSHOTDATA_LOCATION_COMPLETE"},
                {IOCTL_PERSISTENTCBT_DATA, "IOCTL_PERSISTENTCBT_DATA"},
                {IOCTL_PRINTSTATE, "IOCTL_PRINTSTATE"},
                {VEEAMSNAP_CHARCMD_UNDEFINED, "VEEAMSNAP_CHARCMD_UNDEFINED"},
                {VEEAMSNAP_CHARCMD_ACKNOWLEDGE, "VEEAMSNAP_CHARCMD_ACKNOWLEDGE"},
                {VEEAMSNAP_CHARCMD_INVALID, "VEEAMSNAP_CHARCMD_INVALID"},
                {VEEAMSNAP_CHARCMD_INITIATE, "VEEAMSNAP_CHARCMD_INITIATE"},
                {VEEAMSNAP_CHARCMD_NEXT_PORTION, "VEEAMSNAP_CHARCMD_NEXT_PORTION"},
                {VEEAMSNAP_CHARCMD_NEXT_PORTION_MULTIDEV, "VEEAMSNAP_CHARCMD_NEXT_PORTION_MULTIDEV"},
                {VEEAMSNAP_CHARCMD_HALFFILL, "VEEAMSNAP_CHARCMD_HALFFILL"},
                {VEEAMSNAP_CHARCMD_OVERFLOW, "VEEAMSNAP_CHARCMD_OVERFLOW"},
                {VEEAMSNAP_CHARCMD_TERMINATE, "VEEAMSNAP_CHARCMD_TERMINATE"},
		{0, NULL}
	};

	int i = 0;
	printf("test\n");
	while ((ioctl_calls[i].command != 0) && (ioctl_calls[i].name != NULL)) {
		printf("%s: %llu\n", ioctl_calls[i].name, ioctl_calls[i].command);
		i ++;
	}
}
*/



struct ioctl_getversion_s* get_veeam_version(int veeamsfd) {
    struct ioctl_getversion_s* version = (
        struct ioctl_getversion_s*) malloc(sizeof(struct ioctl_getversion_s));
    if (version == NULL) {
        printf("Error occurred while allocating memory for version string.\n");
        return NULL;
    }

    printf("Attempting to read version info from device.\n");
    int result = ioctl(veeamsfd, IOCTL_GETVERSION, version);
    if (result != 0) {
        printf("Error code returned whilst attempting to read version: %d\n", result);
        free(version);
        return NULL;
    }

    return version;
}


int add_device_for_tracking(int veeamsfd, struct ioctl_dev_id_s* device_id) {
    printf(
        "Attempting to register following device for tracking: %d:%d\n",
        device_id->major, device_id->minor);
    int res = ioctl(veeamsfd, IOCTL_TRACKING_ADD, device_id);
    if (res != 0) {
        printf(
            "WARN: failed to enable tracking for device %d:%d with error code: %d\n",
            device_id->major, device_id->minor, res);
    }
    return res;
}

int create_snapstore_for_device(int veeamsfd, char snapstore_id[16], struct ioctl_dev_id_s* device_id) {
    struct ioctl_snapstore_create_s* snapstore_create_params = (struct ioctl_snapstore_create_s *) malloc(sizeof(struct ioctl_snapstore_create_s));
    if (snapstore_create_params <= 0) {
        printf("Failed to allocate memory for snapstore creation IOCTL params.\n");
        return -1;
    }
    strncpy(snapstore_create_params->id, snapstore_id, 16);
    snapstore_create_params->count = 1;
    snapstore_create_params->p_dev_id = device_id;

    int res = ioctl(veeamsfd, IOCTL_SNAPSTORE_CREATE, snapstore_create_params);
    if (res != 0) {
        printf(
            "WARN: failed to create snapstore for device %d:%d. code was: %d\n",
            device_id->major, device_id->minor, res);
    }

    free(snapstore_create_params);
    return res;
}


int set_memory_limit_for_snapstore(int veeamsfd, char snapstore_id[16], unsigned int size_bytes) {
    struct ioctl_snapstore_memory_limit_s * snapstore_memory_limit = (struct ioctl_snapstore_memory_limit_s *) malloc(sizeof(struct ioctl_snapstore_memory_limit_s));
    if (snapstore_memory_limit <= 0) {
        printf("Failed to allocate memory for snapstore memory limit IOCTL params.\n");
        return -1;
    }
    strncpy(snapstore_memory_limit->id, snapstore_id, 16);
    snapstore_memory_limit->size = size_bytes;

    int res = ioctl(veeamsfd, IOCTL_SNAPSTORE_CREATE, snapstore_memory_limit);
    if (res != 0) {
        printf(
            "WARN: failed to set memory limit for snapstore %16s. code was: %d\n",
            snapstore_id, res);
    }

    free(snapstore_memory_limit);
    return res;
}


int create_snapshot_for_device(int veeamsfd, struct ioctl_dev_id_s* device_id) {
    struct ioctl_snapshot_create_s * snapshot_create_params = (struct ioctl_snapshot_create_s *) malloc(sizeof(struct ioctl_snapshot_create_s));
    if (snapshot_create_params <= 0) {
        printf("Failed to allocate memory for snapshot creation IOCTL params.\n");
        return -1;
    }
    snapshot_create_params->p_dev_id = device_id;

    int res = ioctl(veeamsfd, IOCTL_SNAPSHOT_CREATE, snapshot_create_params);
    if (res != 0) {
        printf(
            "WARN: failed to create snapshot for device %d:%d. Error code: %d\n",
            device_id->major, device_id->minor, res);
    }

    free(snapshot_create_params);
    return res;
}

int get_unresolved_kernel_entries(int veeamsfd, struct ioctl_get_unresolved_kernel_entries_s* unres_entries) {
    int res = ioctl(veeamsfd, IOCTL_GET_UNRESOLVED_KERNEL_ENTRIES, unres_entries);
    if (res < 0) {
        printf("WARN: failed to get unresolved kernel entries: %d\n", res);
    }
    return res;
}

int set_kernel_entries(int veeamsfd, struct ioctl_set_kernel_entries_s* ke_entries) {
    int res = ioctl(veeamsfd, IOCTL_SET_KERNEL_ENTRIES, ke_entries);
    if (res < 0) {
        printf("WARN: failed to set kernel entries: %d\n", res);
    }
    return res;
}

int main(int argc, char* argv[]) {
    int veeamsfd = open(VEEAM_SNAP_DEVICE_PATH, O_WRONLY);
    char snapstore_id[16] = "f18992a6225e4e7e";
    if (veeamsfd < 0) {
        printf(
            "Failed to open Veeam device %s. Code: %d\n",
            VEEAM_SNAP_DEVICE_PATH, veeamsfd);
        return veeamsfd;
    }

    // do {
        /*
        struct ioctl_getversion_s* version = get_veeam_version(veeamsfd);
        if (version == NULL) {
            printf("Version get failed.\n");
            // close(fd);
            // return -1;
        } else {
            printf(
                "Read version was: Major-Minor-Revision-Build: %d-%d-%d-%d\n",
                version->major, version->minor, version->revision, version->build);
        }
        */

    // hardcoded /dev/sdb
    struct ioctl_dev_id_s sdb_device_id = {252, 0};
    int res = add_device_for_tracking(veeamsfd, &sdb_device_id);
    if (res != 0) {
        printf(
            "Error code returned whilst trying to add device %d:%d for tracking: %d\n",
            sdb_device_id.major, sdb_device_id.minor, res);

        // try resolving kernel missing kernel entries
        struct ioctl_get_unresolved_kernel_entries_s unres_entries = { "" };
        res = get_unresolved_kernel_entries(veeamsfd, &unres_entries);
        if (res < 0) {
            printf("Error code returned whilst trying to get unresolved kernel entries: %d\n", res);
        } else if (res == 0) {
            printf("No unresolved kernel entries found\n");
        } else {
            printf("Unresolved entry: %s\n", unres_entries.buf);
            struct kernel_entry_s req_module_entry = { 0xffffffff9931c830, KERNEL_ENTRY_BASE_NAME };
            struct kernel_entry_s unres_entry = { 0xffffffff9969dd70, unres_entries.buf };
            struct kernel_entry_s entries[2] = {req_module_entry, unres_entry};
            struct ioctl_set_kernel_entries_s kernel_entries = { 2, entries };
            res = set_kernel_entries(veeamsfd, &kernel_entries);
            if (res < 0) {
                printf("Error code returned whilst trying to set unresolved kernel entries: %d\n", res);
            } else {
                // Kernel entries set, retry
                res = add_device_for_tracking(veeamsfd, &sdb_device_id);
                if (res != 0) {
                    printf(
                        "Error code returned whilst trying to add device %d:%d for tracking: %d\n",
                        sdb_device_id.major, sdb_device_id.minor, res);
                }
            }
        }
    }

        /*
        struct ioctl_dev_id_s sdb_device_id = {8, 16};
        // create snapstore:
        int res = create_snapstore_for_device(veeamsfd, snapstore_id, &sdb_device_id);
        if (res != 0) {
            printf(
                "Error code returned whilst trying to enable snapstore for device %d:%d: %d\n",
                sdb_device_id.major, sdb_device_id.minor, res);
            break;
        } else {
            printf("Successfully enabled snapstore for device %d:%d\n", sdb_device_id.major, sdb_device_id.minor);
        }

        // set snapstore memory limits:
        res = set_memory_limit_for_snapstore(veeamsfd, snapstore_id, 1024 * 1024 * 1024);
        if (res != 0) {
            printf(
                "Error code returned whilst attempting to set memory limit for snapstore %16s: %d\n", snapstore_id, res);
            break;
        } else {
            printf("Successfully set memory limit for snapstore %16s\n", snapstore_id);
        }

        // create snapshot:
        res = create_snapshot_for_device(veeamsfd, &sdb_device_id);
        if (res != 0) {
            printf(
                "Error code returned whilst attempting to create snapshot for device %d:%d: %d\n", sdb_device_id.major, sdb_device_id.major, res);
            break;
        } else {
            printf("Successfully created snapshot for device %d:%d\n", sdb_device_id.major, sdb_device_id.minor);
        }
        */
    // } while(1);

    // free(version);
    close(veeamsfd);
    return 0;
}
