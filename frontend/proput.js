var connection
var devices

var overlay

class Connection {
  constructor(onopen, unsolicited) {
    try {
//      this.socket = new WebSocket("wss://proput.jmq.cz/wss/")
      this.socket = new WebSocket("ws://192.168.1.120:8099/wss/")
      this.socket.onopen = onopen
      this.socket.onmessage = this.recv
      this.socket.onerror = function() {
	overlay.fatalError("Conection failed. Reload to retry.")
      }
    } catch (err) {
      overlay.fatalError("Connection failed: " + err + " Reload to retry.")
      return
    }
    this.lastmsgID = 0
    this.pending = []
    this.unsolicited = unsolicited
  }

  send(o, onreply) {
    var oo = Object(o)
    oo.msgID = ( this.lastmsgID += 1 )
    var j = JSON.stringify(o)
    this.pending.push({ id: o.msgID, obj: o, onreply: onreply })
    this.socket.send(j)
    console.log("SENT: " + j)
  }

  recv(m) {
    console.log("RECV: " + m.data)
    var j = JSON.parse(m.data)
    if (j.msgID > 0) {
      if (connection.pending.length == 0) {
	overlay.fatalError("Unexpected server response, no request pending")
	return
      }

      var p = connection.pending.shift()
      if (p.id != j.msgID)
      {
	overlay.fatalError("Garbled server response, got ID " + j.msgID + ", expected " + p.id)
	return
      }

      p.onreply(p, j)
    } else {
      connection.unsolicited(j)
    }
  }
}

class DeviceList {
  constructor () {
    this.devmap = {}
    this.node = document.getElementById("devices")
    this.updateState()
  }

  updateState() {
    connection.send({ request: "devicelist" }, function (_, data) { devices.recvList(data) })
  }

  recvList(data) {
    console.log("this")
    console.log(this)
    this.devmap = data.devices
    var pending = new Set()
    for (var d in this.devmap)
      pending.add(d)

    var children = []
    for (var i = this.node.childNodes.length; i--; )
      if (this.node.childNodes[i].nodeType == Node.ELEMENT_NODE)
	children.unshift(this.node.childNodes[i])

    for (var n in children) {
      console.log(n)
      if (n.getAttribute("data-device-name") in pending) {
	pending.remove(n)
	/* TODO: update the device */
      } else {
	this.node.removeChild(n)
      }
    }

    console.log(pending)
    console.log(pending.keys())

    for (let d of pending.keys()) {
      console.log(d)
      var dev = this.devmap[d]
      console.log(dev)

      var nameTextNode = document.createTextNode(dev.displayName)
      var nameNode = document.createElement("span")
      nameNode.classList.add("device-name")
      nameNode.appendChild(nameTextNode)

      var devNode = document.createElement("div")
      devNode.setAttribute("data-device-name", dev.name)
      devNode.classList.add("device")
      devNode.appendChild(nameNode)

      this.node.appendChild(devNode)
    }
  }
}

class Overlay {
  constructor () {
    this.node = document.getElementById("overlay")
  }

  fatalError(msg) {
    console.log("Fatal error: " + msg)
    var msgNode = document.createElement("p")
    var textNode = document.createTextNode(msg)
    msgNode.appendChild(textNode)
    this.node.appendChild(msgNode)
    this.node.classList.remove("overlay-hide")
  }
}

function init()
{
  overlay = new Overlay()

  connection = new Connection(
    function() {
      devices = new DeviceList()
    },
    function() {
    }
  )

  document.getElementById("container").classList.toggle("inactive")
}

var initWait = function () {
  if (document.readyState == 'complete')
    setTimeout(init, 50)
  else
    setTimeout(initWait, 50)
}

window.onload = initWait
