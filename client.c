#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>

#define MAX_MESSAGE_LENGTH 256
#define MAX_NAME_LENGTH 50
#define MAX_CARD_NUMBER_LENGTH 20
#define MAX_ORDERS 100

void displayMenu() {
    printf("Welcome to the Coffee Shop!\n");
    printf("Please choose an option:\n");
    printf("1. Espresso: 3.50$\n");
    printf("2. Cappuccino: 4.00$\n");
    printf("3. Latte: 3.80$\n");
    printf("4. Americano: 3.20$\n");
    printf("5. Exit\n");
}

void addMilkOption(char *message) {
    char milkChoice;
    printf("Do you want to add milk to your order? (y/n): ");
    scanf(" %c", &milkChoice);
    if (milkChoice == 'y' || milkChoice == 'Y') {
        strcat(message, " with milk");
    }
}

void handlePayment(char *message, float price) {
    int paymentMethod;
    while (1) {
        printf("Choose payment method:\n");
        printf("1. Cash\n");
        printf("2. Card\n");
        printf("Enter your choice: ");
        scanf("%d", &paymentMethod);
        getchar(); // Clear newline character from buffer

        if (paymentMethod == 2) {
            char cardNumber[MAX_CARD_NUMBER_LENGTH];
            printf("Enter your card number: ");
            fgets(cardNumber, sizeof(cardNumber), stdin);
            cardNumber[strcspn(cardNumber, "\n")] = 0; // Remove newline

            int cardLength = strlen(cardNumber);
            for (int i = 4; i < cardLength - 4; i++) {
                cardNumber[i] = '*';
            }

            char expiryDate[10];
            int valid = 0;
            while (!valid) {
                printf("Enter card expiry date (MM/YY): ");
                fgets(expiryDate, sizeof(expiryDate), stdin);
                expiryDate[strcspn(expiryDate, "\n")] = 0; // Remove newline

                struct tm tm;
                time_t t = time(NULL);
                struct tm *currentDate = localtime(&t);
                int currentYear = currentDate->tm_year + 1900;
                int currentMonth = currentDate->tm_mon + 1;
                int expMonth, expYear;
                if (sscanf(expiryDate, "%d/%d", &expMonth, &expYear) == 2 &&
                    expMonth >= 1 && expMonth <= 12) {
                    expYear += 2000; // Convert to 4-digit year
                    if (expYear > currentYear || (expYear == currentYear && expMonth >= currentMonth)) {
                        valid = 1; // Valid expiry date
                    }
                }

                if (!valid) {
                    printf("Error: Expired or invalid card. Please try again.\n");
                }
            }

            snprintf(message + strlen(message), MAX_MESSAGE_LENGTH, ", Card Number: %s, Expiry Date: %s", cardNumber, expiryDate);
            printf("Payment processed using card: %s\n", cardNumber);
            break;
        } else if (paymentMethod == 1) {
            printf("Payment received in cash.\n");
            break;
        } else {
            printf("Invalid choice. Please select a valid payment method.\n");
        }
    }

    snprintf(message + strlen(message), MAX_MESSAGE_LENGTH, ", Total Price: %.2f", price);
}

float getPrice(int choice) {
    switch (choice) {
        case 1: return 3.50; // Espresso price
        case 2: return 4.00; // Cappuccino price
        case 3: return 3.80; // Latte price
        case 4: return 3.20; // Americano price
        default: return 0.0;
    }
}

int main() {
    int sock;
    struct sockaddr_in server;
    char name[MAX_NAME_LENGTH];

    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        printf("Could not create socket\n");
        return 1;
    }

    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons(50000);

    // Connect to server
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Connection failed");
        return 1;
    }

    printf("Enter your name: ");
    if (fgets(name, sizeof(name), stdin) != NULL) {
        name[strcspn(name, "\n")] = 0; // Remove newline
    } else {
        printf("Error reading name.\n");
        close(sock);
        return 1;
    }

    int choice;
    char *allOrders = malloc(MAX_MESSAGE_LENGTH * MAX_ORDERS * sizeof(char));
    if (allOrders == NULL) {
        perror("Memory allocation failed");
        close(sock);
        return 1;
    }
    allOrders[0] = '\0'; // Initialize to an empty string

    while (1) {
        displayMenu();
        printf("Enter your choice: ");
        scanf("%d", &choice);
        getchar(); // Clear the newline character from the buffer

        if (choice == 5) {
            break; // Exit the loop and send all orders to the server
        }

        // Handle user choices
        float price = getPrice(choice);
        if (price == 0.0) {
            printf("Invalid choice. Please try again.\n");
            continue;
        }

        // Initialize message
        char orderMessage[MAX_MESSAGE_LENGTH];
        snprintf(orderMessage, sizeof(orderMessage), "Order: %s",
                 choice == 1 ? "Espresso" :
                 choice == 2 ? "Cappuccino" :
                 choice == 3 ? "Latte" : "Americano");

        addMilkOption(orderMessage);

        int quantity;
        printf("Enter quantity for your order: ");
        scanf("%d", &quantity);
        getchar(); // Clear newline character
        snprintf(orderMessage + strlen(orderMessage), MAX_MESSAGE_LENGTH - strlen(orderMessage), ", Quantity: %d", quantity);

        // Add order time and client name
        time_t currentTime = time(NULL);
        struct tm *tm_info = localtime(&currentTime);
        char timeStr[9];
        strftime(timeStr, sizeof(timeStr), "%H:%M:%S", tm_info);
        snprintf(orderMessage + strlen(orderMessage), MAX_MESSAGE_LENGTH - strlen(orderMessage), ", Order Time: %s", timeStr);
        snprintf(orderMessage + strlen(orderMessage), MAX_MESSAGE_LENGTH - strlen(orderMessage), ", Client Name: %s", name);

        // Add payment details
        handlePayment(orderMessage, price * quantity);

        // Append the order message to the overall message
        if (strlen(allOrders) + strlen(orderMessage) + 1 > MAX_MESSAGE_LENGTH * MAX_ORDERS) {
            printf("Too many orders. Sending current batch.\n");
            break;
        }
        strncat(allOrders, orderMessage, MAX_MESSAGE_LENGTH * MAX_ORDERS - strlen(allOrders) - 1);
        strcat(allOrders, "\n");

        printf("Order added. You can place another order or type '5' to finish.\n");
    }

    // Send all orders to the server
    if (send(sock, allOrders, strlen(allOrders), 0) < 0) {
        perror("Send failed");
        free(allOrders);
        close(sock);
        return 1;
    }

   // Wait for server's acknowledgment or disconnection
    char serverResponse[MAX_MESSAGE_LENGTH];
    int len = recv(sock, serverResponse, sizeof(serverResponse) - 1, 0);
    if (len > 0) {
        serverResponse[len] = '\0'; // Null-terminate the received string
        printf("%s\n", "");
    } else if (len != 0) {
        printf("Server disconnected: You took too long to respond. Goodbye!\n");
    } else {
        perror("Error processing the order:");
    }

    free(allOrders);
    close(sock);
    return 0;
}
