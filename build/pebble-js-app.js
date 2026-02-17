/******/ (function(modules) { // webpackBootstrap
/******/ 	// The module cache
/******/ 	var installedModules = {};
/******/
/******/ 	// The require function
/******/ 	function __webpack_require__(moduleId) {
/******/
/******/ 		// Check if module is in cache
/******/ 		if(installedModules[moduleId])
/******/ 			return installedModules[moduleId].exports;
/******/
/******/ 		// Create a new module (and put it into the cache)
/******/ 		var module = installedModules[moduleId] = {
/******/ 			exports: {},
/******/ 			id: moduleId,
/******/ 			loaded: false
/******/ 		};
/******/
/******/ 		// Execute the module function
/******/ 		modules[moduleId].call(module.exports, module, module.exports, __webpack_require__);
/******/
/******/ 		// Flag the module as loaded
/******/ 		module.loaded = true;
/******/
/******/ 		// Return the exports of the module
/******/ 		return module.exports;
/******/ 	}
/******/
/******/
/******/ 	// expose the modules object (__webpack_modules__)
/******/ 	__webpack_require__.m = modules;
/******/
/******/ 	// expose the module cache
/******/ 	__webpack_require__.c = installedModules;
/******/
/******/ 	// __webpack_public_path__
/******/ 	__webpack_require__.p = "";
/******/
/******/ 	// Load entry module and return exports
/******/ 	return __webpack_require__(0);
/******/ })
/************************************************************************/
/******/ ([
/* 0 */
/***/ (function(module, exports, __webpack_require__) {

	__webpack_require__(1);
	module.exports = __webpack_require__(2);


/***/ }),
/* 1 */
/***/ (function(module, exports) {

	/**
	 * Copyright 2024 Google LLC
	 *
	 * Licensed under the Apache License, Version 2.0 (the "License");
	 * you may not use this file except in compliance with the License.
	 * You may obtain a copy of the License at
	 *
	 *     http://www.apache.org/licenses/LICENSE-2.0
	 *
	 * Unless required by applicable law or agreed to in writing, software
	 * distributed under the License is distributed on an "AS IS" BASIS,
	 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	 * See the License for the specific language governing permissions and
	 * limitations under the License.
	 */
	
	(function(p) {
	  if (!p === undefined) {
	    console.error('Pebble object not found!?');
	    return;
	  }
	
	  // Aliases:
	  p.on = p.addEventListener;
	  p.off = p.removeEventListener;
	
	  // For Android (WebView-based) pkjs, print stacktrace for uncaught errors:
	  if (typeof window !== 'undefined' && window.addEventListener) {
	    window.addEventListener('error', function(event) {
	      if (event.error && event.error.stack) {
	        console.error('' + event.error + '\n' + event.error.stack);
	      }
	    });
	  }
	
	})(Pebble);


/***/ }),
/* 2 */
/***/ (function(module, exports) {

	const DEBUG = true;
	let stops = {};
	
	let iota_val = 1
	function iota() {
	    let current_iota = iota_val;
	    iota_val += 1;
	    return current_iota;
	}
	
	const REQUEST_NEARBY_STOPS = iota();
	const REQUEST_STOP_DETAILS = iota();
	const POST_NEARBY_STOP = iota();
	
	Pebble.addEventListener('ready', 
	  function(e) {
	    console.log('PebbleKit JS ready!');
	
	    Pebble.sendAppMessage({"JS_READY": 1});
	  }
	);
	
	Pebble.addEventListener("appmessage",
	    function(e) {
	        console.log("melding mottat!")
	        switch(e.payload.MSG_TYPE) {
	            case REQUEST_NEARBY_STOPS: 
	                get_stops_nearby();
	                break;
	            default:
	                console.log("Ikke søttet melding type: ", e.payload.MSG_TYPE);
	        }
	    }
	)
	
	
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
	                maximumDistance: 2000
	                maximumResults: 10
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
	            
	            stops = {};
	            res.data.nearest.edges.forEach((e, i) => {                
	                let stop = {
	                    index: i,
	                    id: e.node.place.id,
	                    name: e.node.place.name,
	                    latitude: e.node.place.latitude,
	                    longitude: e.node.place.longitude,
	                    transportMode: e.node.place.transportMode,
	                    distance: Math.round(e.node.distance)
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
	                        console.log(`sendt stop number ${i}`);
	                        send_stops(stops, i + 1);
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
	            lat: DEBUG ? 59.912126 : pos.coords.latitude,
	            lon: DEBUG ? 10.762378 : pos.coords.longitude
	        }
	    }));
	}

/***/ })
/******/ ]);
//# sourceMappingURL=pebble-js-app.js.map