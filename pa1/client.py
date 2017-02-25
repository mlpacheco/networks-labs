import socket
import sys
import os

BUFFER_SIZE = 10000000
FIXED_PORT = 6000

def main():

    if len(sys.argv) != 4:
        print "Run: {0} [server_host] [server_port] [filename]".format(sys.argv[0])
        return

    # input data needed
    server_hostname = sys.argv[1]
    server_port = int(sys.argv[2])
    if server_port == FIXED_PORT:
        print "port is reserved for establishing connections"
        return

    filename = sys.argv[3]

    # request message according to the HTTP protocol
    if not filename.startswith('/'):
        request = "GET /{0} HTTP/1.1\r\n".format(filename)
    else:
        request = "GET {0} HTTP/1.1\r\n".format(filename)


    # connect, send request and receive response
    sd = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    try:
        sd.connect((server_hostname, FIXED_PORT))
    except socket.error, msg:
        print "Couldn't connect: %s\n" % msg
        return

    sd.send(request)
    sd.close()

    sd_client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    #client_host = socket.gethostname()
    # line below is here to test on my personal laptop
    # with no resolvable hostname
    client_host = '127.0.0.1'
    sd_client.bind((client_host, server_port))
    sd_client.listen(1)
    conn, addr = sd_client.accept()
    data = conn.recv(BUFFER_SIZE)

    conn.close()
    sd_client.close()

    if data:

        # create filename
        filename_final = filename[1:]
        if filename == "/":
            filename_final = "index.html"


        # create path to output file
        # I am assuming the Upload folder will be in
        # the same directory as this source file
        output = os.path.join("Download", filename_final)
        print output

        # write contents
        with open(output, 'w') as f:
            f.write(data)
            f.close()


if __name__ == '__main__':
    main()
