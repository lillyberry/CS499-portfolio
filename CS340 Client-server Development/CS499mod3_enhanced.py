## Setup the Jupyter version of Dash
from jupyter_dash import JupyterDash
from dash import dcc, html, dash_table
from dash.dependencies import Input, Output
import dash_leaflet as dl
import plotly.express as px
import pandas as pd
import base64
import requests

# Flask server for RESTful API
from flask import Flask, jsonify, request
from AnimalShelter import AnimalShelter

# -----------------------------
# Database Connection
# -----------------------------
username = "aacuser"
password = "SNHU1234"
db = AnimalShelter(username, password)

# -----------------------------
# Flask API Setup
# -----------------------------
server = Flask(__name__)

# Dummy user session for RBAC demonstration
current_user = {"username": "lilly", "role": "admin"}  # Change "admin" -> "user" to test RBAC

@server.route('/api/animals', methods=['GET'])
def get_animals():
    query = request.args.to_dict()
    if "age_upon_outcome_in_weeks" in query:
        query["age_upon_outcome_in_weeks"] = {"$lt": int(query["age_upon_outcome_in_weeks"])}
    results = db.read(query)
    return jsonify(results)

@server.route('/api/animals', methods=['POST'])
def add_animal():
    if current_user["role"] != "admin":
        return jsonify({"error": "Unauthorized"}), 403
    data = request.json
    db.create(data)
    return jsonify({"status": "success"})

# Additional endpoints for PUT/DELETE can be added similarly

# -----------------------------
# Dash App Setup
# -----------------------------
app = JupyterDash(__name__, server=server)

# Load initial data from MongoDB
df = pd.DataFrame.from_records(db.read({}))
if "_id" in df.columns:
    df.drop(columns=["_id"], inplace=True)

# Encode logo image
image_filename = 'grazioso.png'
encoded_image = base64.b64encode(open(image_filename, 'rb').read()).decode()

# -----------------------------
# Helper Functions
# -----------------------------
def fetch_animals(filter_type="all"):
    # Define rescue filters
    rescue_filters = {
        "Water Rescue": {"breed": ["Labrador Retriever Mix", "Chesapeake Bay Retriever", "Newfoundland"],
                         "sex_upon_outcome": {"$regex": "Intact Female"},
                         "age_upon_outcome_in_weeks": {"$gte": 26, "$lte": 156}},
        "Mountain or Wilderness Rescue": {"breed": ["German Shepherd", "Alaskan Malamute", "Old English Sheepdog", "Siberian Husky", "Rottweiler"],
                                          "sex_upon_outcome": {"$regex": "Intact Male"},
                                          "age_upon_outcome_in_weeks": {"$gte": 26, "$lte": 156}},
        "Disaster or Individual Tracking": {"breed": ["Doberman Pinscher", "German Shepherd", "Golden Retriever", "Bloodhound", "Rottweiler"],
                                            "sex_upon_outcome": {"$regex": "Intact Male"},
                                            "age_upon_outcome_in_weeks": {"$gte": 20, "$lte": 300}}
    }

    if filter_type == 'all':
        query = {"age_upon_outcome_in_weeks": {"$lt": 104}}
    else:
        query = {
            "breed": {"$in": rescue_filters[filter_type]["breed"]},
            "sex_upon_outcome": rescue_filters[filter_type]["sex_upon_outcome"],
            "age_upon_outcome_in_weeks": rescue_filters[filter_type]["age_upon_outcome_in_weeks"]
        }

    response = requests.get("http://localhost:8051/api/animals", params={"age_upon_outcome_in_weeks": 104 if filter_type=="all" else None})
    data = response.json()
    df_filtered = pd.DataFrame.from_records(data)
    if "_id" in df_filtered.columns:
        df_filtered.drop(columns=["_id"], inplace=True)
    return df_filtered

def generate_pie_chart(df):
    breed_counts = df['breed'].value_counts().reset_index()
    breed_counts.columns = ['Breed', 'Count']
    threshold = 1
    total_count = breed_counts['Count'].sum()
    breed_counts['Percentage'] = (breed_counts['Count'] / total_count) * 100
    small_breeds = breed_counts[breed_counts['Percentage'] < threshold]
    other_count = small_breeds['Count'].sum()
    top_n = 10
    breed_counts = breed_counts.sort_values('Count', ascending=False)
    if len(breed_counts) > top_n:
        breed_counts = breed_counts.iloc[:top_n]
        breed_counts = breed_counts.append({'Breed': 'Other', 'Count': other_count}, ignore_index=True)
    fig = px.pie(breed_counts, names='Breed', values='Count', title="Breed Distribution (Top 10 + Other)", color_discrete_sequence=px.colors.qualitative.Set3)
    fig.update_traces(textposition='inside', textinfo='percent+label')
    return fig

def generate_map(df):
    markers = [
        dl.Marker(
            position=[row.get('latitude', 30.75), row.get('longitude', -97.48)],
            children=[dl.Tooltip(row.get('breed', 'Unknown')),
                      dl.Popup([html.H1(row.get('name', 'Unknown')),
                                html.P(f"Age: {row.get('age_upon_outcome', 'Unknown')}")])]
        )
        for _, row in df.iterrows()
    ]
    return dl.Map(style={'width': '1000px', 'height': '500px'}, center=[30.75, -97.48], zoom=10,
                  children=[dl.TileLayer()] + markers)

# -----------------------------
# Layout
# -----------------------------
app.layout = html.Div([
    html.Center(html.B(html.H1('CS-340 Dashboard'))),
    html.Img(src=f"data:image/png;base64,{encoded_image}", style={'width': '200px'}),
    html.P("Dashboard by: Lilly"),
    html.Hr(),

    html.Label("Filter by Rescue Type"),
    dcc.Dropdown(
        id='filter-type',
        options=[{'label': t, 'value': t} for t in ["All", "Water Rescue", "Mountain or Wilderness Rescue", "Disaster or Individual Tracking"]],
        value='All',
        multi=False
    ),
    html.Br(),

    dash_table.DataTable(
        id='datatable-id',
        columns=[{"name": i, "id": i} for i in df.columns],
        data=df.to_dict('records'),
        page_size=10,
        style_table={'overflowX': 'auto'},
        row_selectable='single',
        selected_rows=[0]
    ),
    html.Br(),
    html.Hr(),
    html.Div(className='row', style={'display': 'flex'}, children=[
        html.Div(id='graph-id', className='col s12 m6'),
        html.Div(id='map-id', className='col s12 m6')
    ])
])

# -----------------------------
# Callbacks
# -----------------------------
@app.callback(
    Output('datatable-id', 'data'),
    Input('filter-type', 'value')
)
def update_table(filter_type):
    df_filtered = fetch_animals(filter_type)
    return df_filtered.to_dict('records')

@app.callback(
    Output('graph-id', 'children'),
    Input('datatable-id', 'derived_virtual_data')
)
def update_graph(viewData):
    if not viewData:
        return "No data available"
    df_view = pd.DataFrame.from_dict(viewData)
    if 'breed' not in df_view.columns or df_view['breed'].isnull().all():
        return "No breed data available"
    fig = generate_pie_chart(df_view)
    return dcc.Graph(figure=fig)

@app.callback(
    Output('map-id', 'children'),
    Input('datatable-id', 'derived_virtual_data')
)
def update_map_callback(viewData):
    if not viewData:
        return "No data available"
    df_view = pd.DataFrame.from_dict(viewData)
    return generate_map(df_view)

# -----------------------------
# Run the Dash App
# -----------------------------
app.run_server(mode='inline', debug=True, port=8051)
