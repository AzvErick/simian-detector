import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ApplicationWindow {
    id: root
    visible: true
    width: 800
    height: 700
    minimumWidth: 600
    minimumHeight: 550
    title: "Simian Detector — Teste CISS"
    color: "#FAFAFA"

    // Estado da aplicacao (properties sao reativas ou seja tudo que depende atualiza sozinho)
    property int matrixSize: 6
    property var gridData: []
    property bool isSimian: false
    property bool analyzed: false
    property string resultText: "Aguardando análise..."
    property string resultColor: "#f0f0f0"
    property string resultTextColor: "#333333"
    property var highlightedCells: ({})
    property var matchDetails: []

    // Inicializa a grid quando a janela abre
    Component.onCompleted: {
        initGrid();
        controller.checkHealth();
    }

    // Escuta os signals que vem do C++ (DnaController)
    Connections {
        target: controller

        function onAnalysisComplete(result) {
            if (result.error && result.error !== "") {
                resultText = result.error;
                resultColor = "#FFCDD2";
                resultTextColor = "#B71C1C";
                analyzed = false;
            } else if (result.isSimian) {
                resultText = "SÍMIO DETECTADO!";
                resultColor = "#C8E6C9";
                resultTextColor = "#1B5E20";
                isSimian = true;
                analyzed = true;
                highlightMatches(result.matches);
            } else {
                resultText = "DNA HUMANO";
                resultColor = "#BBDEFB";
                resultTextColor = "#0D47A1";
                isSimian = false;
                analyzed = true;
                matchDetails = [];
            }
        }

        function onStatsReceived(stats) {
            if (stats.error && stats.error !== "") {
                statsDialog.statsText = "Erro: " + stats.error;
            } else {
                statsDialog.statsText =
                    "DNA Símio:   " + stats.countSimian + "\n" +
                    "DNA Humano:  " + stats.countHuman + "\n" +
                    "Total:       " + (stats.countSimian + stats.countHuman) + "\n" +
                    "Proporção:   " + stats.ratio.toFixed(4);
            }
            statsDialog.open();
        }

        function onHealthChecked(health) {
            if (health.status === "offline") {
                statusIndicator.color = "#F44336";
                statusLabel.text = "API Offline";
            } else if (health.status === "healthy") {
                statusIndicator.color = "#4CAF50";
                statusLabel.text = "API Online";
            } else {
                statusIndicator.color = "#FF9800";
                statusLabel.text = "API Degraded";
            }
        }

        function onNetworkError(message) {
            resultText = message;
            resultColor = "#FFCDD2";
            resultTextColor = "#B71C1C";
        }
    }

    // Funcoes JS

    // Cria grid vazia NxN
    function initGrid() {
        var data = [];
        for (var i = 0; i < matrixSize; i++) {
            var row = [];
            for (var j = 0; j < matrixSize; j++) {
                row.push("");
            }
            data.push(row);
        }
        gridData = data;
        highlightedCells = {};
        analyzed = false;
        resultText = "Aguardando análise...";
        resultColor = "#f0f0f0";
        resultTextColor = "#333333";
        matchDetails = [];
    }

    // Carrega um exemplo aleatorio de DNA
    function loadExample() {
        var examples = {
            4: [
                ["AAAA", "CCCC", "TTTT", "GGGG"],
                ["ATCG", "ATCG", "ATCG", "ATCG"],
                ["AATC", "GCTA", "TTTT", "CGCG"]
            ],
            6: [
                ["CTGAGA", "CTGAGC", "TATTGT", "AGAGGG", "CCCCTA", "TCACTG"],
                ["ATGCGA", "CAGTGC", "TTATGT", "AGAAGG", "CCCCTA", "TCACTG"],
                ["AAAAAA", "TTTTTT", "CCCCCC", "GGGGGG", "ATCGAT", "CGTAGC"],
                ["TGCAGT", "GCATCG", "AAAAGT", "TTTTCG", "CCCCGA", "GGGGTA"]
            ],
            8: [
                ["ATGCATGC", "CGTACGTA", "AAAACCCC", "TTTTGGGG", "ATCGATCG", "GCTAGCTA", "AAAATTTT", "CCCCGGGG"],
                ["AAAAAAAA", "TTTTTTTT", "CCCCCCCC", "GGGGGGGG", "ATCGATCG", "GCTAGCTA", "TACGTACG", "CGATCGAT"]
            ]
        };

        var candidates = examples[matrixSize];

        // Se nao tem exemplo pro tamanho atual, pega um tamanho aleatorio
        if (!candidates) {
            var keys = Object.keys(examples);
            var pickedKey = keys[Math.floor(Math.random() * keys.length)];
            matrixSize = parseInt(pickedKey);
            candidates = examples[pickedKey];
        }

        var chosen = candidates[Math.floor(Math.random() * candidates.length)];
        var size = matrixSize;

        var data = [];
        for (var i = 0; i < size; i++) {
            var row = [];
            for (var j = 0; j < size; j++) {
                row.push(chosen[i][j]);
            }
            data.push(row);
        }
        gridData = data;
        highlightedCells = {};
        analyzed = false;
        resultText = "Exemplo carregado. Clique em 'Analisar DNA'.";
        resultColor = "#FFF9C4";
        resultTextColor = "#F57F17";
        matchDetails = [];
    }

    // Coleta o DNA da grid e manda pro controller
    function analyzeDna() {
        var dnaList = [];
        for (var i = 0; i < matrixSize; i++) {
            var row = "";
            for (var j = 0; j < matrixSize; j++) {
                var cell = gridData[i][j];
                row += (cell && cell.length > 0) ? cell.toUpperCase() : " ";
            }
            dnaList.push(row);
        }
        highlightedCells = {};
        matchDetails = [];
        controller.analyzeDna(dnaList);
    }

    // Monta o mapa de celulas destacadas a partir dos matches
    // Cada celula guarda os indices das sequencias que passam por ela
    function highlightMatches(matches) {
        var cells = {};
        var details = [];

        var dirNames = {
            "horizontal": "Horizontal →",
            "vertical": "Vertical ↓",
            "diagonal_down": "Diagonal ↘",
            "diagonal_up": "Diagonal ↙"
        };

        for (var m = 0; m < matches.length; m++) {
            var match = matches[m];
            var dRow = 0, dCol = 0;

            if (match.endRow > match.startRow) dRow = 1;
            if (match.endRow < match.startRow) dRow = -1;
            if (match.endCol > match.startCol) dCol = 1;
            if (match.endCol < match.startCol) dCol = -1;

            for (var step = 0; step < 4; step++) {
                var r = match.startRow + dRow * step;
                var c = match.startCol + dCol * step;
                var cellKey = r + "," + c;
                if (!cells[cellKey]) cells[cellKey] = [];
                cells[cellKey].push(m);
            }

            details.push(
                "Seq " + (m + 1) + ": " + match.letter + " " +
                (dirNames[match.direction] || match.direction) +
                " [" + match.startRow + "," + match.startCol + "] → " +
                "[" + match.endRow + "," + match.endCol + "]"
            );
        }

        // Lista celulas com sobreposicao (mais de uma sequencia passa)
        var overlapFound = false;
        for (var key in cells) {
            if (cells[key].length > 1) {
                if (!overlapFound) {
                    details.push("─── Sobreposições ───");
                    overlapFound = true;
                }
                var coords = key.split(",");
                var seqLabels = [];
                for (var si = 0; si < cells[key].length; si++) {
                    seqLabels.push("Seq " + (cells[key][si] + 1));
                }
                details.push("  [" + coords[0] + "," + coords[1] + "] → " + seqLabels.join(" + "));
            }
        }

        highlightedCells = cells;
        matchDetails = details;
    }

    readonly property var cellPalette: ["#81C784", "#FFB74D", "#64B5F6", "#F06292", "#BA68C8"]

    function getCellOverlapCount(row, col) {
        var key = row + "," + col;
        if (!highlightedCells.hasOwnProperty(key)) return 0;
        return highlightedCells[key].length;
    }

    function getCellColors(row, col) {
        var key = row + "," + col;
        if (!highlightedCells.hasOwnProperty(key)) return [];
        var indices = highlightedCells[key];
        var result = [];
        for (var i = 0; i < indices.length; i++) {
            result.push(cellPalette[indices[i] % cellPalette.length]);
        }
        return result;
    }

    // Layout da interface

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        // Barra superior, status da API + controles
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            // Bolinha de status
            Rectangle {
                id: statusIndicator
                width: 12; height: 12
                radius: 6
                color: "#9E9E9E"
            }

            Label {
                id: statusLabel
                text: "Verificando..."
                font.pixelSize: 12
                color: "#757575"
            }

            Item { Layout.fillWidth: true }

            Label { text: "Tamanho (N):"; font.pixelSize: 14 }

            SpinBox {
                id: sizeSpinner
                from: 4; to: 20
                value: matrixSize
                onValueModified: {
                    matrixSize = value;
                    initGrid();
                }
            }

            Button {
                text: "Carregar Exemplo"
                onClicked: loadExample()
                background: Rectangle {
                    color: parent.hovered ? "#E3F2FD" : "#F5F5F5"
                    border.color: "#BDBDBD"
                    radius: 4
                }
            }

            Button {
                text: "Limpar"
                onClicked: initGrid()
                background: Rectangle {
                    color: parent.hovered ? "#FFEBEE" : "#F5F5F5"
                    border.color: "#BDBDBD"
                    radius: 4
                }
            }
        }

        // Grid do DNA
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "white"
            border.color: "#E0E0E0"
            border.width: 1
            radius: 8

            GridLayout {
                anchors.fill: parent
                anchors.margins: 8
                columns: matrixSize
                rowSpacing: 2
                columnSpacing: 2

                // Cria N*N celulas
                Repeater {
                    model: matrixSize * matrixSize

                    Rectangle {
                        property int row: Math.floor(index / matrixSize)
                        property int col: index % matrixSize
                        property alias inputField: cellInput

                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumWidth: 30
                        Layout.minimumHeight: 30

                        color: "white"
                        border.color: getCellOverlapCount(row, col) > 1 ? "#E53935" : "#E0E0E0"
                        border.width: getCellOverlapCount(row, col) > 1 ? 2 : 1
                        radius: 4
                        clip: true

                        // Canvas pinta o fundo da celula
                        // Se tem sobreposicao de sequencias, divide em faixas coloridas
                        Canvas {
                            anchors.fill: parent
                            anchors.margins: 1
                            property var matchColors: getCellColors(row, col)
                            onMatchColorsChanged: requestPaint()
                            Component.onCompleted: requestPaint()

                            onPaint: {
                                var ctx = getContext("2d");
                                var w = width, h = height;
                                ctx.clearRect(0, 0, w, h);
                                var colors = matchColors;

                                if (colors.length === 0) {
                                    ctx.fillStyle = "white";
                                    ctx.fillRect(0, 0, w, h);
                                } else if (colors.length === 1) {
                                    ctx.fillStyle = colors[0];
                                    ctx.fillRect(0, 0, w, h);
                                } else {
                                    // Sobreposicao, faixas diagonais com cor de cada sequencia
                                    var n = colors.length;
                                    var sliceWidth = w / n;
                                    for (var i = 0; i < n; i++) {
                                        ctx.fillStyle = colors[i];
                                        ctx.beginPath();
                                        var x0 = i * sliceWidth;
                                        var x1 = (i + 1) * sliceWidth;
                                        ctx.moveTo(x0, 0);
                                        ctx.lineTo(x1, 0);
                                        ctx.lineTo(x1 + h, h);
                                        ctx.lineTo(x0 + h, h);
                                        ctx.closePath();
                                        ctx.fill();
                                    }
                                }
                            }
                        }

                        // Badge de sobreposicao (aparece quando 2+ sequencias passam pela celula)
                        Rectangle {
                            visible: getCellOverlapCount(row, col) > 1
                            width: 15; height: 15
                            radius: 8
                            color: "#E53935"
                            anchors.top: parent.top
                            anchors.right: parent.right
                            anchors.topMargin: 1
                            anchors.rightMargin: 1
                            z: 3

                            Text {
                                anchors.centerIn: parent
                                text: getCellOverlapCount(row, col).toString()
                                font.pixelSize: 8
                                font.bold: true
                                color: "white"
                            }
                        }

                        // Campo de texto da celula só aceita 1 letra (A/T/C/G)
                        TextInput {
                            id: cellInput
                            anchors.centerIn: parent
                            width: parent.width - 4
                            horizontalAlignment: TextInput.AlignHCenter
                            font.pixelSize: Math.min(parent.width, parent.height) * 0.5
                            font.bold: true
                            font.family: "Courier"
                            maximumLength: 1
                            color: "#212121"
                            z: 2

                            text: (gridData[row] && gridData[row][col]) ? gridData[row][col] : ""
                            validator: RegExpValidator { regExp: /[ATCGatcg]/ }

                            onTextEdited: {
                                if (gridData[row]) {
                                    var newData = gridData.slice();
                                    var newRow = newData[row].slice();
                                    newRow[col] = text.toUpperCase();
                                    newData[row] = newRow;
                                    gridData = newData;
                                }

                                // Pula pra proxima celula automaticamente
                                if (text.length === 1) {
                                    var nextIndex = index + 1;
                                    if (nextIndex < matrixSize * matrixSize) {
                                        var nextItem = parent.parent.children[nextIndex];
                                        if (nextItem && nextItem.inputField) {
                                            nextItem.inputField.forceActiveFocus();
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // Botoes
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Button {
                Layout.fillWidth: true
                Layout.preferredHeight: 48
                text: controller.loading ? "Analisando..." : "Analisar DNA"
                enabled: !controller.loading
                font.pixelSize: 16
                font.bold: true
                onClicked: analyzeDna()

                background: Rectangle {
                    color: parent.enabled ? (parent.hovered ? "#45a049" : "#4CAF50") : "#A5D6A7"
                    radius: 8
                }
                contentItem: Text {
                    text: parent.text; font: parent.font; color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }

            Button {
                Layout.fillWidth: true
                Layout.preferredHeight: 48
                text: "Estatísticas"
                font.pixelSize: 16
                onClicked: controller.fetchStats()

                background: Rectangle {
                    color: parent.hovered ? "#1976D2" : "#2196F3"
                    radius: 8
                }
                contentItem: Text {
                    text: parent.text; font: parent.font; color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }

        // Resultado
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            color: resultColor
            radius: 8

            Label {
                anchors.centerIn: parent
                text: resultText
                font.pixelSize: 20
                font.bold: true
                color: resultTextColor
            }
        }

        // Detalhes das sequencias encontradas
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(detailsText.implicitHeight + 20, 130)
            color: "white"
            border.color: "#E0E0E0"
            border.width: 1
            radius: 8
            clip: true
            visible: matchDetails.length > 0 || (!analyzed && matchDetails.length === 0)

            ScrollView {
                id: detailsScroll
                anchors.fill: parent
                anchors.margins: 8
                clip: true
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                Text {
                    id: detailsText
                    width: detailsScroll.width
                    text: matchDetails.length > 0
                        ? matchDetails.join("\n")
                        : "Detalhes das sequências aparecerão aqui..."
                    font.pixelSize: 12
                    font.family: "Courier"
                    color: matchDetails.length > 0 ? "#333333" : "#9E9E9E"
                    wrapMode: Text.WordWrap
                }
            }
        }
    }

    // Dialog de estatisticas
    Dialog {
        id: statsDialog
        title: "Estatísticas de Verificação"
        modal: true
        anchors.centerIn: parent
        width: 350
        property string statsText: ""

        ColumnLayout {
            spacing: 12
            Label { text: "=== DNA Verification Statistics ==="; font.bold: true; font.pixelSize: 14 }
            Label { text: statsDialog.statsText; font.family: "Courier"; font.pixelSize: 13 }
        }
        standardButtons: Dialog.Ok
    }
}
