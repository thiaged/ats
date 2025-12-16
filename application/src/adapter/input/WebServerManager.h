#pragma once
#include <ESPAsyncWebServer.h>
#include <core/domain/BatterySource.h>
#include <ESPmDNS.h>
#include <core/domain/EnergySource.h>
#include <Ticker.h>
#include <adapter/output/ScreenManager.h>
#include <Update.h>
#include <adapter/output/ScreenUpdate.h>
#include <PubSubClient.h>

#define RELAY_PIN 39
#define MQTT_ID "ats-thiaged"
#define MQTT_USER "mqtt_user"
#define MQTT_PASSWORD "123@123"

const char menu_html[] PROGMEM = R"rawliteral(
    <nav class="nav-menu">
        <ul class="menu-list">
            <li class="menu-item %CLASS_HOME_ACTIVE%"><a href="/">Home</a></li>
            <li class="menu-item %CLASS_PREFERENCE_ACTIVE%"><a href="/preference">Preferências</a></li>
            <li class="menu-item %CLASS_SOBRE_ACTIVE%"><a href="/about">Sobre</a></li>
        </ul>
    </nav>
    )rawliteral";

const char form_action_html[] PROGMEM = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
        <title>ATS thiaged</title>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <meta charset="UTF-8">
        %CSS_HTML%
    </head>
    <body>
        <div class="main-wrapper">
            %MENU%

            <div class="container">
                <h4 style="text-align: center;">Salvo com sucesso</h4>
            </div>
        </div>
    </body>
    </html>
    )rawliteral";

const char css_html[] PROGMEM = R"rawliteral(
    <style>
        .main-wrapper {
            width: 100%;
            max-width: 900px; /* Mesma largura do container */
            margin: 0 auto; /* Centraliza tudo */
            padding: 20px;
            box-sizing: border-box;
        }
        /* Novo CSS do Menu */
        .main-wrapper .nav-menu {
            width: 100%;
            max-width: 900px; /* Igual ao container */
            background: #ffffff;
            border-radius: 18px;
            box-shadow: 0 4px 25px rgba(0, 0, 0, 0.1);
            margin-bottom: 20px; /* Espaço antes do container */
            gap: 20px;
            display: grid;
            position: relative;
        }

        .main-wrapper .nav-menu .menu-list {
            list-style: none;
            margin: 0;
            padding: 1rem 20px;
            display: flex;
            flex-wrap: wrap;
            gap: 1rem;
            justify-content: center;
        }

        .main-wrapper .nav-menu .menu-list .menu-item a {
            text-decoration: none;
        }

        .main-wrapper .nav-menu .menu-list .menu-item.active a {
            color: white;
        }

        .main-wrapper .nav-menu .menu-list .menu-item {
            color: #1e88e5;
            font-weight: 600;
            font-size: small;
            text-transform: uppercase;
            letter-spacing: 1px;
            cursor: pointer;
            transition: all 0.3s;
            padding: 0.5rem 1rem;
            border-radius: 8px;
        }

        .main-wrapper .nav-menu .menu-list .menu-item:hover {
            background: #f0f7ff;
            transform: translateY(-2px);
        }

        .main-wrapper .nav-menu .menu-list .menu-item.active {
            background: #1e88e5;
            color: white;
        }

        html, body {
            max-width: 100%;
        }

        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            margin: 0;
            padding: 0;
            display: flex;
            flex-direction: column;
            align-items: center; /* Centralização horizontal */
            min-height: 100vh;
            background: #f5f8fa;
        }

        .container {
            margin: 0;
            position: relative;
            width: 100%;
            max-width: 900px;
            height: auto;
            background: #ffffff;
            border-radius: 18px;
            box-shadow: 0 4px 25px rgba(0, 0, 0, 0.1);
            display: grid;
            grid-template-rows: auto 1fr auto;
            gap: 20px;
        }

        .header {
            display: flex;
            justify-content: space-between;
            align-items: center;
        }

        .component {
            text-align: center;
            padding: 15px;
            border-radius: 12px;
            transition: transform 0.3s;
            width: 100px;
            height: 100px;
            max-width: 140px;
            max-height: 140px;
        }

        .component:hover {
            transform: scale(1.05) translateY(-3px);
        }

        .component .label {
            font-weight: 600;
            text-transform: uppercase;
            letter-spacing: 1px;
            font-size: 12px;
            color: #1e88e5;
        }

        .component svg {
            width: 100%;
            height: auto;
            fill: inherit;
            stroke: inherit;
            color: inherit;
        }

        .house {
            text-align: center;
            padding: 0;
            border-radius: 12px;
            width: 150px;
            height: 150px;
            margin: 0 auto;
        }

        .footer {
            display: flex;
            justify-content: space-between;
            gap: 20px;
            margin-bottom: 50px;
        }

        svg.connections {
            z-index: 0;
            position: absolute;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            pointer-events: none;
        }

        .connection {
            stroke-width: 8px;
            stroke-linecap: round;
            stroke-dasharray: 12;
            animation: none;
        }

        .conection-active {
            animation: flow 0.6s linear infinite;
        }

        .conection-inactive {
            animation: none;
            stroke: #f0f7ff !important;
        }

        @keyframes flow {
            0% { stroke-dashoffset: 60; }
            100% { stroke-dashoffset: 0; }
        }

        /* Cores das linhas */
        .solar-line { stroke: #4CAF50; }
        .utility-line { stroke: #e36161; }
        .battery-line { stroke: #bdd078; }

        /* Media Queries */
        @media (max-width: 768px) {
            .main-wrapper .nav-menu .menu-list .menu-list {
                flex-direction: column;
                align-items: center;
                gap: 1rem;
            }

            .main-wrapper .nav-menu .menu-list .menu-item {
                width: 100%;
                text-align: center;
            }


            .component {
                width: 80px;
                height: 80px;
            }

            .house {
                width: 120px;
                height: 120px;
            }

            .connection {
                stroke-width: 8px;
            }
        }

        @media (max-width: 480px) {
            .container {
                gap: 10px;
            }

            .component {
                width: 60px;
                height: 60px;
            }

            .house {
                width: 100px;
                height: 100px;
            }

            .connection {
                stroke-width: 8px;
            }
        }


        .form-section {
            margin-bottom: 2rem;
            padding: 1.5rem;
            border-radius: 12px;
            background: #f8f9fa;
        }

        .form-section-title {
            display: flex;
            align-items: center;
            gap: 10px;
            color: #1e88e5;
            margin-bottom: 1.5rem;
        }

        .form-row {
            display: flex;
            gap: 20px;
            margin-bottom: 1.5rem;
        }

        .input-group {
            flex: 1;
            text-align: center;
        }

        label {
            display: block;
            margin-bottom: 8px;
            font-weight: 600;
            color: #2c3e50;
        }

        input, select {
            width: 100%;
            padding: 12px 0;
            border: 2px solid #e0e0e0;
            border-radius: 8px;
            background: #ffffff;
            transition: all 0.3s;
            text-align: center;
        }

        input:focus, select:focus {
            border-color: #1e88e5;
            box-shadow: 0 0 8px rgba(30, 136, 229, 0.2);
            outline: none;
        }

        .button-group {
            display: flex;
            justify-content: flex-end;
            gap: 15px;
            margin-top: 2rem;
        }

        button {
            padding: 12px 30px;
            border: none;
            border-radius: 8px;
            cursor: pointer;
            font-weight: 600;
            text-transform: uppercase;
            letter-spacing: 1px;
            transition: all 0.3s;
        }

        .btn-primary {
            background: #1e88e5;
            color: white;
        }
        .btn-primary:disabled {
            background: #e0e0e0;
            color:rgb(64, 64, 64);
        }

        .btn-secondary {
            background: #f8f9fa;
            color: #2c3e50;
            border: 2px solid #e0e0e0;
        }

        button:hover {
            transform: translateY(-2px);
            box-shadow: 0 4px 15px rgba(0, 0, 0, 0.1);
        }

        @media (max-width: 768px) {
            .form-row {
                flex-direction: column;
            }

            .form-section {
                padding: 1rem;
            }
        }
    </style>
    )rawliteral";

// Página HTML com formulário POST
const char preference_html[] PROGMEM = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
        <title>ATS thiaged</title>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <meta charset="UTF-8">
        %CSS_HTML%
    </head>
    <body>
        <div class="main-wrapper">
            %MENU%

            <div class="container settings-form">
                <form action="/preference" method="post">
                    <div class="form-section">
                        <div class="form-section-title">
                            <i class="material-icons">Geral</i>
                            <h2>Configurações Gerais</h2>
                        </div>
                        <div class="form-row">
                            <div class="input-group">
                                <label>Transferir após sincronia das redes</label>
                                <select name="waitSyncStatus">
                                    <option value="0" %check_wait_sinc_status_off%>OFF</option>
                                    <option value="1" %check_wait_sinc_status_on%>ON</option>
                                </select>
                            </div>
                            <div class="input-group">
                                <label>Calibração voltagem AC</label>
                                <input name="calib-ac" value="%calib-ac%" type="number" step="0.00001" placeholder="Calibração da voltagem" />
                            </div>
                        </div>
                    </div>
                    <div class="form-section">
                        <div class="form-section-title">
                            <i class="material-icons">Bateria</i>
                            <h2>Configurações de bateria</h2>
                        </div>

                        <div class="form-row">
                            <div class="input-group">
                                <label>BMS comunication</label>
                                <select name="bmsStatus">
                                    <option value="0" %BMSCONFIGSELECTOFF%>OFF</option>
                                    <option value="1" %BMSCONFIGSELECTON%>ON</option>
                                </select>
                            </div>
                            <div class="input-group">
                                <label>Tensão baixa que deve mudar de solar para CEMIG</label>
                                <input name="batteryConfigMin" value="%batteryConfigMin%" type="number" step="0.01" placeholder="Valor em voltagem" />
                                <input name="batteryConfigMinPercentage" value="%batteryConfigMinPercentage%" type="number" step="1" placeholder="Valor em porcentagem" />
                            </div>
                            <div class="input-group">
                                <label>Tensão baixa que deve mudar de CEMIG para solar</label>
                                <input name="batteryConfigMax" value="%batteryConfigMax%" type="number" step="0.01" placeholder="Valor em voltagem" />
                                <input name="batteryConfigMaxPercentage" value="%batteryConfigMaxPercentage%" type="number" step="1" placeholder="Valor em porcentagem" />
                            </div>
                        </div>

                    </div>
                    <!-- Botões de Ação -->
                    <div class="button-group">
                        <button type="submit" class="btn-primary">Salvar Configurações</button>
                    </div>
                </form>

            </div>

        </div>

    </body>
    </html>
    )rawliteral";

// Página HTML com formulário POST
const char about_html[] PROGMEM = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
        <title>ATS thiaged</title>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <meta charset="UTF-8">
        %CSS_HTML%
    </head>
    <body>
        <div class="main-wrapper">
            %MENU%
            <div class="container settings-form">
                <form action="/about" method="POST" enctype="multipart/form-data">
                    <div class="form-section">
                        <div class="form-row">
                            <div class="input-group">
                                <label>Novo firmware - tipo .bin</label>
                                <input name="data" type="file" accept=".bin">
                            </div>
                            <div id="progress"></div>
                        </div>
                    </div>
                    <!-- Botões de Ação -->
                    <div class="button-group">
                        <button type="submit" class="btn-primary" onClick="this.disabled=true; this.innerText='Processando...'; this.form.submit();">Atualizar firmware</button>
                    </div>
                </form>
            </div>
        </div>
        <script>
        if (!!window.EventSource) {
            var source = new EventSource('/events');

            source.addEventListener('open', function(e) {
                console.log("Events Connected");
            }, false);

            source.addEventListener('progress', function(e) {
                document.getElementById("progress").innerHTML = e.data + "%";
                console.log("myevent", e.data);
            }, false);
        }
        </script>
    </body>
    </html>
    )rawliteral";

const char index_html[] PROGMEM = R"rawliteral(
    <!DOCTYPE html>
    <html lang="pt-BR">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>ATS thiaged</title>
        %CSS_HTML%
    </head>
    <body>
        <div class="main-wrapper">
            %MENU%
            <div class="container">
                <!-- SVG Connections -->
                <svg class="connections" viewBox="0 0 900 500" preserveAspectRatio="none">
                    <!-- Solar para Casa -->
                    <path id="solarStatus" class="connection solar-line conection-inactive" d="M 170,90 Q 350,100 430,150" fill="none"></path>
                    <!-- Rede para Casa -->
                    <path id="utilityStatus" class="connection utility-line conection-inactive" d="M 760,90 Q 550,100 480,150" fill="none"></path>
                    <!-- Bateria para Casa -->
                    <path id="batteryStatus" class="connection battery-line conection-inactive" d="M 170,410 Q 350,380 400,300" fill="none"></path>
                </svg>
                <!-- Header Components -->
                <div class="header">
                    <div class="component solar">
                        <div class="icon-energy"><svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 141 141" style="width: 100%; height: auto;">
                            <g><g data-cell-id="0"><g data-cell-id="1"><g data-cell-id="XV2GXwhh_PNB0a5Yqk5h-1"/><g data-cell-id="XV2GXwhh_PNB0a5Yqk5h-2"><g><ellipse cx="70" cy="70" rx="44.21052631578947" ry="44.21052631578947" fill="#ffffff" stroke="#000000" pointer-events="all" style="fill: #f2bc6c; stroke: #000;"/><path d="M 62.63 21.37 L 70 0 L 77.37 21.37 Z" fill="#ffffff" stroke="#000000" stroke-miterlimit="10" pointer-events="all" style="fill: #bbe17a; stroke: #000;"/><path d="M 62.63 118.63 L 70 140 L 77.37 118.63 Z" fill="#ffffff" stroke="#000000" stroke-miterlimit="10" pointer-events="all" style="fill: #bbe17a; stroke: #000;"/><path d="M 21.37 62.63 L 0 70 L 21.37 77.37 Z" fill="#ffffff" stroke="#000000" stroke-miterlimit="10" pointer-events="all" style="fill: #bbe17a; stroke: #000;"/><path d="M 118.63 62.63 L 140 70 L 118.63 77.37 Z" fill="#ffffff" stroke="#000000" stroke-miterlimit="10" pointer-events="all" style="fill: #bbe17a; stroke: #000;"/><path d="M 99.47 30.21 L 119.66 20.48 L 109.79 40.53 Z" fill="#ffffff" stroke="#000000" stroke-miterlimit="10" pointer-events="all" style="fill: #bbe17a; stroke: #000;"/><path d="M 99.47 109.79 L 119.66 119.52 L 109.79 99.47 Z" fill="#ffffff" stroke="#000000" stroke-miterlimit="10" pointer-events="all" style="fill: #bbe17a; stroke: #000;"/><path d="M 40.53 30.21 L 20.34 20.48 L 30.21 40.53 Z" fill="#ffffff" stroke="#000000" stroke-miterlimit="10" pointer-events="all" style="fill: #bbe17a; stroke: #000;"/><path d="M 40.53 109.79 L 20.34 119.52 L 30.21 99.47 Z" fill="#ffffff" stroke="#000000" stroke-miterlimit="10" pointer-events="all" style="fill: #bbe17a; stroke: #000;"/></g></g><g data-cell-id="XV2GXwhh_PNB0a5Yqk5h-3"/><g data-cell-id="XV2GXwhh_PNB0a5Yqk5h-4"/></g></g></g>
                        </svg></div>
                        <div class="label">Solar</div>
                        <div id="solarVoltage" class="voltage-display">%solarVoltage%V</div>
                    </div>
                    <div class="component utility">
                        <div><svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 97 142" style="width: 100%; height: auto;">
                            <g><g data-cell-id="0"><g data-cell-id="1"><g data-cell-id="XV2GXwhh_PNB0a5Yqk5h-1"/><g data-cell-id="XV2GXwhh_PNB0a5Yqk5h-2"/><g data-cell-id="XV2GXwhh_PNB0a5Yqk5h-3"><g><path d="M 9.32 141 L 48.5 24.1 L 87.68 141" fill="none" stroke="#ffffff" stroke-width="2.83" stroke-linejoin="round" stroke-miterlimit="10" pointer-events="all" style="stroke: #000;"/><path d="M 85.9 133.21 L 20 109.83 L 71.65 94.24 L 31.58 78.65 L 60.97 63.77 L 40.49 50.31" fill="none" stroke="#ffffff" stroke-width="2.83" stroke-linejoin="round" stroke-miterlimit="10" pointer-events="all" style="stroke: #000;"/><rect x="1" y="1" width="0" height="0" fill="none" stroke="#ffffff" stroke-width="2.83" pointer-events="all" style="stroke: #000;"/><ellipse cx="48.5" cy="26.93" rx="7.12410948631421" ry="5.668016194331984" fill="#ffffff" stroke="#ffffff" stroke-width="2.83" pointer-events="all" style="fill: #0c0b0b; stroke: #000;"/><path d="M 60.61 17.3 L 63.1 14.89 C 71.25 21.5 71.25 32.08 63.1 38.69 L 60.61 36.43 C 66.84 31.01 66.84 22.71 60.61 17.3 Z M 69.52 10.21 L 72.01 8.23 C 84.68 18.66 84.68 35.2 72.01 45.64 L 69.52 43.65 C 80.83 34.32 80.83 19.54 69.52 10.21 Z M 78.42 3.13 L 80.91 1 C 90.49 7.46 96 16.94 96 26.93 C 96 36.92 90.49 46.4 80.91 52.86 L 78.42 50.74 C 87.52 44.96 92.82 36.2 92.82 26.93 C 92.82 17.67 87.52 8.91 78.42 3.13 Z M 36.39 17.3 L 33.9 14.89 C 25.75 21.5 25.75 32.08 33.9 38.69 L 36.39 36.43 C 30.16 31.01 30.16 22.71 36.39 17.3 Z M 27.48 10.21 L 24.99 8.23 C 12.32 18.66 12.32 35.2 24.99 45.64 L 27.48 43.65 C 16.17 34.32 16.17 19.54 27.48 10.21 Z M 18.58 3.13 L 16.09 1 C 6.51 7.46 1 16.94 1 26.93 C 1 36.92 6.51 46.4 16.09 52.86 L 18.58 50.74 C 9.48 44.96 4.18 36.2 4.18 26.93 C 4.18 17.67 9.48 8.91 18.58 3.13 Z" fill="#ffffff" stroke="#ffffff" stroke-width="2.83" stroke-miterlimit="10" pointer-events="all" style="fill: #0c0c0c; stroke: #000;"/></g></g><g data-cell-id="XV2GXwhh_PNB0a5Yqk5h-4"/></g></g></g>
                        </svg></div>
                        <div class="label">Rede Elétrica</div>
                        <div id="utilityVoltage" class="voltage-display">%utilityVoltage%V</div>
                    </div>
                </div>
                <!-- House in Center -->
                <div class="house">
                    <div class="icon-energy"><svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 152 152" style="width: 100%; height: auto;">
                        <g><g data-cell-id="0"><g data-cell-id="1"><g data-cell-id="XV2GXwhh_PNB0a5Yqk5h-1"><g><path d="M 10.38 151 L 10.38 62.76 L 76 9.82 L 141.63 62.76 L 141.63 151 L 94.75 151 L 94.75 106.88 L 57.25 106.88 L 57.25 151 Z" fill="#ffffff" stroke="#0080f0" stroke-width="2" stroke-miterlimit="10" pointer-events="all" style="fill: #4ad9e3; stroke: rgb(0, 128, 240);"/><path d="M 27.25 41.59 L 27.25 18.65 L 38.5 18.65 L 38.5 32.76" fill="#ffffff" stroke="#0080f0" stroke-width="2" stroke-miterlimit="10" pointer-events="all" style="fill: #8e8b82; stroke: rgb(0, 128, 240);"/><path d="M 1 62.76 L 76 1 L 151 62.76" fill="none" stroke="#0080f0" stroke-width="2" stroke-miterlimit="10" pointer-events="all" style="stroke: rgb(0, 128, 240);"/></g></g><g data-cell-id="XV2GXwhh_PNB0a5Yqk5h-2"/><g data-cell-id="XV2GXwhh_PNB0a5Yqk5h-3"/><g data-cell-id="XV2GXwhh_PNB0a5Yqk5h-4"/></g></g></g>
                    </svg></div>
                </div>
                <!-- Footer Component -->
                <div class="footer">
                    <div class="component battery">
                        <div><svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 144 144" style="width: 100%; height: auto;">
                            <g><g data-cell-id="0"><g data-cell-id="1"><g data-cell-id="XV2GXwhh_PNB0a5Yqk5h-1"/><g data-cell-id="XV2GXwhh_PNB0a5Yqk5h-2"/><g data-cell-id="XV2GXwhh_PNB0a5Yqk5h-3"/><g data-cell-id="XV2GXwhh_PNB0a5Yqk5h-4"><g><image x="-0.5" y="-0.5" width="144" height="144" xlink:href="data:image/png;base64,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" preserveAspectRatio="none" transform="rotate(-90,72,72)"/></g></g></g></g></g>
                        </svg></div>
                        <div class="label">Bateria</div>
                        <div id="batteryVoltage" class="voltage-display">%batteryVoltage%V</div>
                    </div>
                </div>
            </div>
        </div>
        <script>
            const ws = new WebSocket('ws://' + window.location.hostname + '/ws');
            ws.onmessage = (event) => {
            const data = JSON.parse(event.data);
            if ("solarVoltage" in data) {
                document.getElementById("solarVoltage").innerHTML = data.solarVoltage + "V";
            }

            if ("utilityVoltage" in data) {
                document.getElementById("utilityVoltage").innerHTML = data.utilityVoltage + "V";
            }

            var usingSource = "";

            if ("usingSource" in data) {
                usingSource = data.usingSource;
            }

            if ("solarStatus" in data) {
                var elem = document.getElementById("solarStatus");
                if (data.solarStatus === "active") {
                    elem.classList = "connection solar-line" + (usingSource === "solar" ? " conection-active" : "");
                } else {
                    elem.classList = "connection solar-line conection-inactive";
                }
            }

            if ("utilityStatus" in data) {
                var elem = document.getElementById("utilityStatus");
                if (data.utilityStatus === "active") {
                    elem.classList = "connection utility-line" + (usingSource === "utility" ? " conection-active" : "");
                } else {
                    elem.classList = "connection utility-line conection-inactive";
                }
            }

            if ("batteryVoltage" in data) {
                document.getElementById("batteryVoltage").innerHTML = data.batteryVoltage + "V";
            }

            if ("statusBms" in data) {
                var elem = document.getElementById("batteryStatus");
                if (data.statusBms == "active") {
                    elem.classList = "connection battery-line conection-active";
                }
                if (data.statusBms == "inactive") {
                    elem.classList = "connection battery-line conection-inactive";
                }
            }

            };
        </script>
    </body>
    </html>
    )rawliteral";

enum MenuList {
    HOME,
    PREFERENCE,
    ABOUT
};

class WebserverManager
{

private:
    EnergySource &utilitySource;
    EnergySource &solarSource;
    BatterySource &battery;
    ScreenManager &screenManager;
    ScreenUpdate &screenUpdate;
    bool &waitForSync;
    bool &firmwareUpdateUtilityOn;
    Preferences &configPreferences;

    EnergySource **userDefinedSource;
    bool *userSourceLocked;
    bool *updatingFirmware;

    unsigned long &inactivityTime;

    AsyncWebServer server;
    AsyncWebSocket websocket;
    AsyncEventSource events;

    WiFiClient client;
    PubSubClient mqttClient;
    // MQTT reconnection helpers
    unsigned long lastMqttReconnectAttempt;
    unsigned long mqttReconnectIntervalMs;


    String setMenuEnabled(String html, MenuList menu);

    void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
        AwsEventType type, void *arg, uint8_t *data, size_t len);

    void onMqttMessage(char *topic, byte *payload, unsigned int length);

    void subscribeAllMqttTopics();
    // Try to reconnect to MQTT if needed (non-blocking, respects interval)
    void attemptMqttReconnect();

public:
    WebserverManager(
        EnergySource &pUtilitySource,
        EnergySource &pSolarSource,
        BatterySource &pBattery,
        ScreenManager &pScreenManager,
        ScreenUpdate &pScreenUpdate,
        EnergySource **pUserDefinedSource,
        bool *pUserSourceLocked,
        bool *pUpdatingFirmware,
        bool &pWaitForSync,
        Preferences &pConfigPreferences,
        unsigned long &pInactivityTime,
        bool &pFirmwareUpdateUtilityOn
    );
    virtual ~WebserverManager() = default;
    void Init();
    void NotifyClients(const String &message);
    void NotifyClientsEvent(const char *message, const char *eventName);
    bool HasWsClients();
    void SendToMqtt(const char *topic, const char *message);
    void LoopMqtt();
    void SetWaitForSync(bool wait);
};