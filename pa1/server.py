import socket
import sys
import os
import re

BUFFER_SIZE = 10000000

# validate http request by extracting the GET request
# line and parsing it
def validate_http_request(request):
    match = re.search(r'(.*?\r\n)', request)
    if not match:
        return False, None

    firstline = match.group(1)
    splits = firstline.split()

    if  len(splits) != 3 or \
        splits[2] != "HTTP/1.1" or \
        splits[0] != "GET" or \
        not splits[1].startswith("/"):

        return False, None

    filename = splits[1][1:]
    if splits[1] == "/":
        filename = "index.html"

    return True, filename

def main():

    if len(sys.argv) != 2:
        print "Run: {0} [port]".format(sys.argv[0])
        return

    # input data needed
    port = int(sys.argv[1])
    host = socket.gethostname()
    # line below is here to test on my personal laptop
    # with no resolvable hostname
    #host = '127.0.0.1'

    # create and bind socket
    sd = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sd.bind((host, port))
    sd.listen(5)

    # actively listen for requests
    while 1:

        conn, addr = sd.accept()
        data = conn.recv(BUFFER_SIZE)

        # if I did receive a message, proceed
        if data:
            valid, filename = validate_http_request(data)

            if not valid:
                print "HTTP request is incorrect"
                continue

            # I am assuming the Upload folder will be in
            # the same directory as this source file
            output = os.path.join("Upload", filename)
            if not os.path.isfile(output):
                print "File {0} doesn't exists".format(filename)
                continue

            # read contents of file and write them to the client
            with open(output) as f:
                contents = f.read()
                conn.send(contents)



if __name__ == '__main__':
    main()
