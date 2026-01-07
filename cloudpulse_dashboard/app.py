from flask import Flask, jsonify, render_template, request
import mysql.connector
import os
from dotenv import load_dotenv

load_dotenv()

app = Flask(__name__, static_folder='static', template_folder='templates')

def get_db_conn():
    cfg = {
        'host': os.getenv('DB_HOST', '127.0.0.1'),
        'user': os.getenv('DB_USER', 'monitor'),
        'password': os.getenv('DB_PASS', ''),
        'database': os.getenv('DB_NAME', 'cloud_monitor'),
        'port': int(os.getenv('DB_PORT', '3306'))
    }
    return mysql.connector.connect(**cfg)

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/api/metrics/latest')
def api_latest():
    try:
        conn = get_db_conn()
        cur = conn.cursor(dictionary=True)
        cur.execute("SELECT * FROM metrics ORDER BY timestamp DESC LIMIT 1")
        row = cur.fetchone()
        cur.close()
        conn.close()
        if not row:
            return jsonify({'error':'no data'}), 404
        return jsonify(row)
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/api/metrics/history')
def api_history():
    limit = int(request.args.get('limit', '100'))
    try:
        conn = get_db_conn()
        cur = conn.cursor(dictionary=True)
        cur.execute("SELECT * FROM metrics ORDER BY timestamp DESC LIMIT %s", (limit,))
        rows = cur.fetchall()
        cur.close()
        conn.close()
        rows.reverse()
        return jsonify(rows)
    except Exception as e:
        return jsonify({'error': str(e)}), 500

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=int(os.getenv('PORT', '8080')), debug=False)
