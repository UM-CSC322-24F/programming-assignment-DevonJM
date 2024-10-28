#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_BOATS 120
#define MAX_NAME_LENGTH 127

typedef enum {
    SLIP, LAND, TRAILOR, STORAGE
} LocationType;

typedef union {
    int slipNumber;     // For boats in slips
    char bayLetter;     // For boats on land
    char trailorTag[10]; // For boats on trailors
    int storageNumber;  // For boats in storage
} ExtraInfo;

typedef struct {
    char name[MAX_NAME_LENGTH + 1];
    double length;
    LocationType location;
    ExtraInfo extra;
    double amountOwed;
} Boat;

Boat *boats[MAX_BOATS]; // Array of pointers to hold up to 120 boats
int boatCount = 0;

// Function Prototypes
void loadBoatsFromFile(const char *filename);
void saveBoatsToFile(const char *filename);
void displayInventory();
void addBoat(const char *csvData);
void removeBoat(const char *boatName);
void acceptPayment(const char *boatName, double amount);
void updateMonthlyCharges();
void sortBoats();
LocationType getLocationTypeFromString(const char *str);
const char *getLocationTypeAsString(LocationType type);

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <BoatData.csv>\n", argv[0]);
        return 1;
    }

    // Load boats from file
    loadBoatsFromFile(argv[1]);

    char choice;
    char csvData[256], boatName[MAX_NAME_LENGTH + 1];
    double amount;

    printf("Welcome to the Boat Management System\n-------------------------------------\n");

    while (1) {
        printf("(I)nventory, (A)dd, (R)emove, (P)ayment, (M)onth, e(X)it: ");
        scanf(" %c", &choice);
        choice = tolower(choice);

        switch (choice) {
            case 'i':
                displayInventory();
                break;
            case 'a':
                printf("Please enter the boat data in CSV format: ");
                scanf(" %[^\n]", csvData);
                addBoat(csvData);
                break;
            case 'r':
                printf("Please enter the boat name: ");
                scanf(" %[^\n]", boatName);
                removeBoat(boatName);
                break;
            case 'p':
                printf("Please enter the boat name: ");
                scanf(" %[^\n]", boatName);
                printf("Please enter the amount to be paid: ");
                scanf("%lf", &amount);
                acceptPayment(boatName, amount);
                break;
            case 'm':
                updateMonthlyCharges();
                break;
            case 'x':
                printf("Exiting the Boat Management System\n");
                saveBoatsToFile(argv[1]);
                return 0;
            default:
                printf("Invalid option %c\n", choice);
                break;
        }
    }
}


// Load boat data from a CSV file
void loadBoatsFromFile(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open file");
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), file) && boatCount < MAX_BOATS) {
        Boat *boat = malloc(sizeof(Boat));
        if (!boat) {
            fprintf(stderr, "Memory allocation failed\n");
            fclose(file);
            return;
        }

        // Initialize extra buffer to avoid garbage values
        memset(&boat->extra, 0, sizeof(boat->extra));

        // Parse the line and initialize the boat
        char location[10];
        if (sscanf(line, "%127[^,],%lf,%9[^,],%9[^,],%lf", 
                   boat->name, &boat->length, location, 
                   boat->extra.trailorTag, &boat->amountOwed) < 5) {
            fprintf(stderr, "Error parsing line: %s", line);
            free(boat); // Free the allocated boat memory
            continue; 
        }

        boat->location = getLocationTypeFromString(location);
        
        // Ensure correct handling based on the location type
        switch (boat->location) {
            case SLIP:
                sscanf(boat->extra.trailorTag, "%d", &boat->extra.slipNumber);
                break;
            case LAND:
                boat->extra.bayLetter = boat->extra.trailorTag[0];
                break;
            case TRAILOR: 
                // The trailorTag is already set in sscanf
                break;
            case STORAGE:
                sscanf(boat->extra.trailorTag, "%d", &boat->extra.storageNumber);
                break;
            default:
                fprintf(stderr, "Unknown location type for boat: %s\n", boat->name);
                free(boat); // Free the allocated boat memory
                continue; 
        }

        boats[boatCount++] = boat;
    }
    fclose(file);
}


// Save boat data to a CSV file
void saveBoatsToFile(const char *filename) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        perror("Failed to open file for saving");
        return;
    }

    for (int i = 0; i < boatCount; i++) {
        Boat *boat = boats[i];
        
        switch (boat->location) {
            case SLIP:
                fprintf(file, "%s,%.2f,%s,%d", boat->name, boat->length, getLocationTypeAsString(boat->location), boat->extra.slipNumber);
                break;
            case LAND:
                fprintf(file, "%s,%.2f,%s,%c", boat->name, boat->length, getLocationTypeAsString(boat->location), boat->extra.bayLetter);
                break;
            case TRAILOR:
                fprintf(file, "%s,%.2f,%s,%s", boat->name, boat->length, getLocationTypeAsString(boat->location), boat->extra.trailorTag);
                break;
            case STORAGE:
                fprintf(file, "%s,%.2f,%s,%d", boat->name, boat->length, getLocationTypeAsString(boat->location), boat->extra.storageNumber);
                break;
        }
        fprintf(file, ",%.2f\n", boat->amountOwed);
    }

    fflush(file); // Flush the output buffer
    fclose(file);
}



// Display the inventory of boats
void displayInventory() {
    sortBoats();  // Ensure inventory is sorted by name

    printf("\nBoat Inventory:\n");
    printf("--------------------------------------------------------\n");
    for (int i = 0; i < boatCount; i++) {
        Boat *boat = boats[i];
        printf("%-20s %4.0f' %-8s ", boat->name, boat->length, getLocationTypeAsString(boat->location));

        switch (boat->location) {
            case SLIP:
                printf("# %d", boat->extra.slipNumber);
                break;
            case LAND:
                printf("   %c", boat->extra.bayLetter);
                break;
            case TRAILOR:
                printf("%s", boat->extra.trailorTag);
                break;
            case STORAGE:
                printf("# %d", boat->extra.storageNumber);
                break;
        }
        printf("   Owes $%.2f\n", boat->amountOwed);
    }
    printf("--------------------------------------------------------\n\n");
}


// Add a new boat to the inventory
void addBoat(const char *csvData) {
    if (boatCount >= MAX_BOATS) {
        printf("Marina is full, cannot add more boats.\n");
        return;
    }

    Boat *newBoat = malloc(sizeof(Boat));
    if (!newBoat) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }

    char location[10];
    sscanf(csvData, "%127[^,],%lf,%9[^,],%9[^,],%lf",
           newBoat->name, &newBoat->length, location, 
           newBoat->extra.trailorTag, &newBoat->amountOwed);

    newBoat->location = getLocationTypeFromString(location);

    // Handle extra information based on location type
    switch (newBoat->location) {
        case SLIP:
            sscanf(newBoat->extra.trailorTag, "%d", &newBoat->extra.slipNumber);
            break;
        case LAND:
            newBoat->extra.bayLetter = newBoat->extra.trailorTag[0];
            break;
        case TRAILOR:
            // Use trailorTag as is
            break;
        case STORAGE:
            sscanf(newBoat->extra.trailorTag, "%d", &newBoat->extra.storageNumber);
            break;
        default:
            fprintf(stderr, "Unknown location type: %s\n", location);
            free(newBoat);
            return;
    }

    boats[boatCount++] = newBoat;
    printf("Boat added successfully.\n");
}



// Remove a boat from the inventory
void removeBoat(const char *boatName) {
    for (int i = 0; i < boatCount; i++) {
        if (strcasecmp(boats[i]->name, boatName) == 0) {
            free(boats[i]);

            // Shift remaining boats down to keep array packed
            for (int j = i; j < boatCount - 1; j++) {
                boats[j] = boats[j + 1];
            }
            boatCount--;
            printf("Boat '%s' removed successfully.\n", boatName);
            return;
        }
    }
    printf("No boat with that name.\n");
}

// Accept payment for a boat
void acceptPayment(const char *boatName, double amount) {
    for (int i = 0; i < boatCount; i++) {
        if (strcasecmp(boats[i]->name, boatName) == 0) {
            if (amount > boats[i]->amountOwed) {
                printf("That is more than the amount owed, $%.2f\n", boats[i]->amountOwed);
                return;
            }
            boats[i]->amountOwed -= amount;
            printf("Payment accepted. New balance for '%s' is $%.2f\n", boatName, boats[i]->amountOwed);
            return;
        }
    }
    printf("No boat with that name.\n");
}

// Update monthly charges for all boats
void updateMonthlyCharges() {
    for (int i = 0; i < boatCount; i++) {
        double chargePerFoot = 0;

        switch (boats[i]->location) {
            case SLIP:
                chargePerFoot = 12.50;
                break;
            case LAND:
                chargePerFoot = 14.00;
                break;
            case TRAILOR:
                chargePerFoot = 25.00;
                break;
            case STORAGE:
                chargePerFoot = 11.20;
                break;
        }

        boats[i]->amountOwed += chargePerFoot * boats[i]->length;
    }
    printf("Monthly charges updated.\n");
}


// Comparison function for qsort
int compareBoatsByName(const void *a, const void *b) {
    // Cast the void pointers to Boat pointers
    Boat *boatA = *(Boat **)a;
    Boat *boatB = *(Boat **)b;
    
    // Compare names (case insensitive)
    return strcasecmp(boatA->name, boatB->name);
}

// Sort boats by name
void sortBoats() {
    qsort(boats, boatCount, sizeof(Boat *), compareBoatsByName);
}


// Helper functions
LocationType getLocationTypeFromString(const char *str) {
    if (strcasecmp(str, "slip") == 0) return SLIP;
    if (strcasecmp(str, "land") == 0) return LAND;
    if (strcasecmp(str, "trailor") == 0) return TRAILOR;
    if (strcasecmp(str, "storage") == 0) return STORAGE;
    return -1;
}

const char *getLocationTypeAsString(LocationType type) {
    switch (type) {
        case SLIP: return "slip";
        case LAND: return "land";
        case TRAILOR: return "trailor";
        case STORAGE: return "storage";
        default: return "unknown";
    }
}
