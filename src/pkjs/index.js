const DEBUG = true;
let stops = [];

let iota_val = 1
function iota() {
    let current_iota = iota_val;
    iota_val += 1;
    return current_iota;
}

const REQUEST_NEARBY_STOPS                    = iota();
const REQUEST_STOP_DETAILS                    = iota();
const REQUEST_NEARBY_LINES_PER_TRANSPORT_MODE = iota();

const POST_NEARBY_STOP                        = iota();
const POST_LINE_DATA                          = iota();

Pebble.addEventListener('ready', 
  function(e) {
    console.log('PebbleKit JS ready!');

    Pebble.sendAppMessage({"JS_READY": 1});
  }
);

Pebble.addEventListener("appmessage",
    function(e) {
        console.log("melding mottat!", JSON.stringify(e.payload))
        switch(e.payload.MSG_TYPE) {
            case REQUEST_NEARBY_STOPS: 
                get_stops_nearby();
                break;
            case REQUEST_NEARBY_LINES_PER_TRANSPORT_MODE:
                send_lines_per_transportMode(stops[e.payload.STOP_INDEX]);
                break;
            default:
                console.log("Ikke søttet melding type: ", e.payload.MSG_TYPE);
                break;
        }
    }
)

function send_lines_per_transportMode(stop) {
    let line_data = get_lines_per_transportMode(stop);
    
    let send_line_data = (line_data, i) => {



        let data = {
            "MSG_TYPE": POST_LINE_DATA,
            "LINE_TRANSPORT_MODE": line_data[i][0],
            "LINE_CODE": line_data[i][1],
        }

        Pebble.sendAppMessage(data,
            () => {
                console.log(`sendt line number ${i + 1}/${line_data.length}`);
                if (line_data.length > i + 1) {
                    send_line_data(line_data, i + 1);
                } else {
                    console.log("done sending line data");
                }
            },
            e => console.log(`failed to send line number ${i} `, e)
        )
    }

    send_line_data(line_data, 0)
}
        
function get_lines_per_transportMode(stop) {
    let lines_per_transportMode = new Set();
    
    
    stop.quays.forEach(quay => {
        quay.lines.forEach(line => {
            lines_per_transportMode.add(`${line.transportMode}|${line.publicCode}`);
        })
    })


    return Array.from(lines_per_transportMode).map(str => str.split("|"));
}


function get_stops_nearby() {
    navigator.geolocation.getCurrentPosition(
        get_stops_nearby_location_success,
        e => console.log("kunne ikke skaffe posisjon: ", e),
        {timeout: 15000, maximumAge:60000}
    );
}


function get_stops_nearby_location_success(pos) {
    let xhr = new XMLHttpRequest();
    let url = "https://api.entur.io/journey-planner/v3/graphql";

    let query = `
        query GetNearest($lat: Float!, $lon: Float!) {
            nearest(
                latitude: $lat
                longitude: $lon
                filterByPlaceTypes: stopPlace
                filterByInUse: true
                maximumDistance: 2000
                maximumResults: 10
                filterByModes: [bus, tram, rail, metro, water]
            ) {
                edges {
                    node {
                        place {
                        ... on StopPlace {
                            id
                            name
                            latitude
                            longitude
                            transportMode
                            quays(filterByInUse: true) {
                                publicCode
                                name
                                description
                                lines {
                                    name
                                    publicCode
                                    transportMode
                                    presentation {
                                        colour
                                        textColour
                                    }
                                }
                            }
                        }
                    }
                    distance
                    }
                }
            }
        }
    `;

    xhr.open("POST", url, true);
    xhr.setRequestHeader("Content-Type", "application/json");
    xhr.setRequestHeader("ET-Client-Name", "Pebble-test-app");


    xhr.onload = () => {
        if (xhr.status === 200) {
            let res = JSON.parse(xhr.responseText);

            if (res.errors) {
                console.log("GraphQL errors:", response.errors);
                return;
            }
            
            res.data.nearest.edges.forEach((e, i) => {                
                let stop = {
                    index: i,
                    id: e.node.place.id,
                    name: e.node.place.name,
                    latitude: e.node.place.latitude,
                    longitude: e.node.place.longitude,
                    transportMode: e.node.place.transportMode,
                    distance: Math.round(e.node.distance),
                    quays: e.node.place.quays
                };

                stops[i] = stop;
            });

            let send_stops = (stops, i) => {
                let data = {
                    "MSG_TYPE": POST_NEARBY_STOP,
                    "STOP_INDEX": stops[i].index,
                    "STOP_NAME": stops[i].name,
                    "DISTANCE": stops[i].distance
                }

                Pebble.sendAppMessage(data,
                    () => {
                        console.log(`sendt stop number ${i + 1}/${stops.length}`);
                        if (stops.length > i + 1) {
                            send_stops(stops, i + 1);
                        } else {
                            console.log("done sending stops");
                        }
                    },
                    e => console.log(`failed to send stop number ${i} `, e)
                )
            }

            send_stops(stops, 0)

        }
    }

    xhr.onerror = () => {
        console.log("Network error");
    };


    // 59.911430, 10.733166 oslo senter
    // 59.912126, 10.762378 Grønland

    xhr.send(JSON.stringify({
        query: query,
        variables: {
            lat: DEBUG ? 59.911430 : pos.coords.latitude,
            lon: DEBUG ? 10.733166 : pos.coords.longitude
        }
    }));
}