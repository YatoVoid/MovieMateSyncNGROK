from flask import Flask, send_file

app = Flask(__name__)

@app.route('/')
def index():
    # Return the contents of data.txt file
    return send_file('data.txt')

if __name__ == '__main__':
    app.run(debug=True)
