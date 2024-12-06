import java.io.*;
import java.net.*;

public class CoffeeShopServer {
    public static void main(String[] args) {
        try (ServerSocket serverSocket = new ServerSocket(50000)) {
            System.out.println("CoffeeShop Server is listening on port 50000");
             // Timeout for accepting new clients

            while (true) {
                
                    Socket socket = serverSocket.accept();
                    System.out.println("New client connected");

                    // Handle each client in a separate thread
                    new Thread(new ClientHandler(socket)).start();
                
            }
        } catch (IOException e) {
            System.out.println("Server failed: " + e.getMessage());
        }
    }
}

class ClientHandler implements Runnable {
    private Socket socket;

    public ClientHandler(Socket socket) {
        this.socket = socket;
        try {
            // Set read timeout for this client
            this.socket.setSoTimeout(60000); // 15 seconds
        } catch (SocketException e) {
            System.out.println("Error setting socket timeout: " + e.getMessage());
        }
    }

    @Override
    public void run() {
        try {
            InputStream input = socket.getInputStream();
            BufferedReader reader = new BufferedReader(new InputStreamReader(input));

            OutputStream output = socket.getOutputStream();
            PrintWriter writer = new PrintWriter(output, true);

            String message;
            while ((message = reader.readLine()) != null) {
                if ("Exit".equalsIgnoreCase(message)) {
                    break;
                }

                try {
                    // Parse the order message
                    String[] parts = message.split(", ");
                    String itemName = parts[0].split(": ")[1];
                    String quantity = parts[1].split(": ")[1];
                    String orderTime = parts[2].split(": ")[1];
                    String clientName = parts[3].split(": ")[1];
                    String paymentDetails = parts[4];

                    // Display the final order and transferring message
                    System.out.println("Final Order:");
                    System.out.println("Item Name: " + itemName);
                    System.out.println("Quantity: " + quantity);
                    System.out.println("Order Time: " + orderTime);
                    System.out.println("Client Name: " + clientName);
                    System.out.println("Payment Details: " + paymentDetails);

                    System.out.println("Order successfully transferred for: " + clientName);

                    // Optionally send a response to the client
                    writer.println("Order received: " + message);
                } catch (Exception e) {
                    System.out.println("Error processing the order: " + e.getMessage());
                    writer.println("Error processing your order.");
                }
            }
        } catch (SocketTimeoutException e) {
            System.out.println("Client did not send data in the timeout period.");
        } catch (IOException e) {
            System.out.println("");
        } finally {
            try {
                socket.close();
            } catch (IOException e) {
                System.out.println("Error closing client socket: " + e.getMessage());
            }
        }
    }
}
