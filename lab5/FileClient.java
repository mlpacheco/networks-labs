import java.io.*;
import java.net.*;
import java.util.*;

public class FileClient {

    public static void main(String[] args) {
        // check thar we have all needed params
        if (args.length != 5) {
            System.out.println("Run: java FileClient [hostname] [portnumber] " +
                               "[secretkey] [filename] [configfile]");
            return;
        }

        System.out.println(Arrays.toString(args));

        if (args[2].length() < 10 || args[2].length() > 20) {
            System.out.println("Secret key should be of length [10,20]");
            return;
        }

        File f = new File(args[3]);
        if (f.exists()) {
            System.out.println("File already exists");
            return;
        }

        try {
            // create message to be sent and send it to the server
            Socket clientSocket =
                new Socket(args[0], Integer.parseInt(args[1]));
            String message = "$" + args[2] + "$" + args[3];
            PrintWriter outToServer =
                new PrintWriter(clientSocket.getOutputStream(), true);
            //System.out.println("Sending: " + message);
            outToServer.printf(message);

            // read configuration file to know how many bytes to read at a time
            BufferedReader inFromFile =
                new BufferedReader(new FileReader(args[4]));
            String line = inFromFile.readLine();
            int numBytes = Integer.parseInt(line);

            // open file to append downloaded contents
            FileOutputStream outToFile =
                new FileOutputStream(args[3], true);
            DataInputStream inFromServer =
                new DataInputStream(clientSocket.getInputStream());

            byte[] readBytes = new byte[numBytes];
            int numRead;
            int totalRead = 0;

            long startTime = System.currentTimeMillis();
            numRead = inFromServer.read(readBytes, 0, numBytes);
            while (numRead > 0) {
                outToFile.write(readBytes);
                totalRead += numBytes;
                numRead = inFromServer.read(readBytes, 0, numBytes);
            }
            long endTime = System.currentTimeMillis();

            double elapsedSeconds = (endTime - startTime)/1000.0;
            double totalMB = totalRead / 131072.0;
            double reliableThroughput = totalMB / elapsedSeconds;

            System.out.printf("read = %d bytes\ntime = %f "+
                              "sec\nreliable_throughput = %f Mbps\n",
                              totalRead, elapsedSeconds, reliableThroughput);

        } catch (IOException e) {
            //e.printStackTrace();
            System.out.println(e.getMessage());
        }

    }

}
