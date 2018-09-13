'    Copyright (C) 2018 Anton Vakulenko 
'    e-mail: anton.vakulenko@gmail.com
'
'    This file is part of TinyShooter.
'
'    TinyShooter is free software: you can redistribute it and/or modify
'    it under the terms of the GNU General Public License as published by
'    the Free Software Foundation, either version 3 of the License, or
'    (at your option) any later version.
'
'    TinyShooter is distributed in the hope that it will be useful,
'    but WITHOUT ANY WARRANTY; without even the implied warranty of
'    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
'    GNU General Public License for more details.
'
'    You should have received a copy of the GNU General Public License
'    along with TinyShooter.  If not, see <http://www.gnu.org/licenses/>.

Imports System.IO.Ports
Imports System.Threading
Imports System.Runtime.InteropServices
Imports System.Net.Sockets

Public Class FormMain

#Region "global_variables"
    ' Save as class variable, new delegate of event handler
    Public inObjectEventHandler As New EdsObjectEventHandler(AddressOf CameraEventListener.handleObjectEvent)

    ' misc public variables:
    Dim intTotalTime As Integer ' time till the session's end in seconds
    Dim client As TcpClient ' tcp client for PHD server

    ' variables to timer1_tick procedure:
    Dim intCount As Integer = 0 ' number of seconds (ticks) passed
    Dim intFrame As Integer = 0 ' number of frames taken
    Dim strStatus As String = "Mirror" ' initial status for timer1_tick procedure
#End Region

    Private Sub Form1_Load(ByVal sender As Object, ByVal e As System.EventArgs) Handles Me.Load
        ' initial stats
        Statistics((nbExposure.Value + My.Settings.Mirror + My.Settings.Pause) * nbRepeat.Value - My.Settings.Pause, "Idle")
        ' if user didn't enter path for saving images we use application folder
        If My.Settings.Path = "" Then My.Settings.Path = Application.StartupPath

    End Sub

    Public Function ConnectCamera() As String
        ' function for connecting to camera
        ' set public variable blnCameraConnected to true, and returns "OK" if succeed (or any error)

        ' initialize SDK
        Try
            EdsInitializeSDK()
            'Thread.Sleep(500)
        Catch
            ' not connected
            'blnCameraConnected = False
            ConnectCamera = "Error initializing Canon SDK"
            Exit Function
        End Try

        ' get list of connected cameras
        Dim intCameraList As IntPtr
        Try
            EdsGetCameraList(intCameraList)
        Catch
            ' not connected
            'blnCameraConnected = False
            ConnectCamera = "Error getting camera's list"
            Exit Function
        End Try

        ' get first connected camera
        Dim intMyCamera As IntPtr ' pointer to the camera
        Try
            EdsGetChildAtIndex(intCameraList, 0, intMyCamera)
        Catch
            ' not connected
            'blnCameraConnected = False
            ConnectCamera = "Error getting first camera available"
            Exit Function
        End Try

        ' start session with selected camera
        Dim intErr As Integer
        intErr = EdsOpenSession(intMyCamera)
        If intErr <> 0 Then
            ' not connected
            'blnCameraConnected = False
            ConnectCamera = "Error opening session with camera"
            Exit Function
        End If

        ' save images to pc
        Dim intSaveTo As Integer = EdsSaveTo.kEdsSaveTo_Host
        Try
            EdsSetPropertyData(intMyCamera, kEdsPropID_SaveTo, 0, Marshal.SizeOf(intSaveTo), intSaveTo)
        Catch
            ' some error occured, so we assume the camera is not connected
            'blnCameraConnected = False
            ConnectCamera = "Error setting camera to save images on PC"
            Exit Function
        End Try

        ' tell camera that we have a plenty of free space on hdd
        Dim capacity As EdsCapacity
        capacity.numberOfFreeClusters = &H7FFFFFFF
        capacity.bytesPerSector = &H1000
        capacity.reset = 1
        Try
            EdsSetCapacity(intMyCamera, capacity)
        Catch
            ' some error occured, so we assume the camera is not connected
            'blnCameraConnected = False
            ConnectCamera = "Error telling camera PC's capacity"
            Exit Function
        End Try

        ' cool! we're connected to camera!
        'blnCameraConnected = True
        ConnectCamera = "OK"

        ' set event handler to download images from camera's memory to pc
        EdsSetObjectEventHandler(intMyCamera, kEdsObjectEvent_DirItemRequestTransfer, inObjectEventHandler, IntPtr.Zero)

    End Function

    Private Sub ButtonStartStop_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles btnStartStop.Click
        ' this button starts or stops shooting

        ' at first, we need to know which button was pressed (start or stop)
        If btnStartStop.Text = "Start" Then
            ' looks like we want to start shooting
            ' let's prepare for it

            ' get com port name from our settings
            SerialPort1.PortName = My.Settings.port
            ' trying to open com port
            Try
                SerialPort1.Open()
            Catch ex As Exception
                ' Houston, we have a problem
                MsgBox(ex.Message, MsgBoxStyle.Information)
                Exit Sub
            End Try

            ' do we need to download images from camera to pc?
            If My.Settings.Download = True Then
                ' looks like we do
                ' trying to connect to camera
                Dim strCamera As String = ConnectCamera()
                If strCamera <> "OK" Then
                    ' oops, mission failed :(
                    MsgBox(strCamera, MsgBoxStyle.Information)
                    Exit Sub
                    'End If
                End If
            End If

            ' do we need to dither?
            If cbDither.Checked = True Then
                ' trying to connect to PHD
                Try
                    ' Create a TcpClient.
                    client = New TcpClient("127.0.0.1", 4400)
                Catch
                    ' something got wrong, connection failed
                    MsgBox("Can't connect to PHD", MsgBoxStyle.Information)
                    Exit Sub
                End Try
            End If

            ' ititial session dureation for timer ticking statistics
            intTotalTime = (nbExposure.Value + My.Settings.Pause + My.Settings.Mirror) * nbRepeat.Value - My.Settings.Pause

            ' total progressbar max value is equal to total session time in seconds
            ' updating every second (every timer tick)
            ' so 1 step = 1 second
            ProgressBarTotal.Maximum = (nbExposure.Value + My.Settings.Pause + My.Settings.Mirror) * nbRepeat.Value - My.Settings.Pause
            ' set progressbar to 0
            ProgressBarTotal.Value = 0

            ' block menu, exposure and repeat while shooting
            btnOptions.Enabled = False
            nbExposure.Enabled = False
            nbRepeat.Enabled = False

            ' renaming button  
            btnStartStop.Text = "Stop"

            ' it's time to start ticking (all shooting actions take place in timer1_tick)
            Timer1.Start()

        Else
            ' button stop was pressed: we need to abort shooting session
            ' run cleanup() and drop progressbars and Statistics
            CleanUp()
            Statistics((nbExposure.Value + My.Settings.Pause + My.Settings.Mirror) * nbRepeat.Value - My.Settings.Pause, "Aborted")
        End If
    End Sub

    Private Sub Timer1_Tick(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Timer1.Tick
        ' main shooting procedure
        ' at first we should check our status
        ' it could be mirror, capture and pause
        Select Case strStatus
            Case "Mirror"
                If intCount = 0 Then
                    ' shutter button down
                    SerialPort1.RtsEnable = True
                    ' wait (on some systems it's necessary)
                    Thread.Sleep(100)
                    ' shutter button up
                    SerialPort1.RtsEnable = False
                    ' now we just made mirror lock-up
                End If
                ' update statistics
                
                intTotalTime -= 1
                Statistics(intTotalTime, "Mirror")
                intCount += 1
                ' checking if mirror settle time elapsed
                If intCount = My.Settings.Mirror Then
                    ' change status
                    strStatus = "Capture"
                    intCount = 0
                End If
            Case "Capture"
                If intCount = 0 Then
                    ' shutter button down (we open shutter)
                    SerialPort1.RtsEnable = True
                End If
                ' update statistics
                intTotalTime -= 1
                Statistics(intTotalTime, "Capture")
                intCount += 1
                ' checking if exposure time elapsed
                If intCount = My.Settings.Exposure Then
                    ' change status
                    strStatus = "Pause"
                    intCount = 0
                End If

            Case "Pause"
                If intCount = 0 Then
                    ' shutter button up (close shutter, mirror down)
                    SerialPort1.RtsEnable = False
                    ' this is the end of the single frame shooting
                    ' now we say "dither" to PHD (if needed)
                    If cbDither.Checked = True Then Dither()
                End If
                ' update statistics
                intTotalTime -= 1
                Statistics(intTotalTime, "Pause")
                intCount += 1
                ' checking if pause time elapsed
                If intCount = My.Settings.Pause Then
                    intCount = 0
                    ' change status
                    strStatus = "Mirror"
                    intFrame += 1
                    ' checking if we shoot all frames
                    If intFrame = My.Settings.Repeat Then
                        intFrame = 0
                        ' call this to stop our shooting session
                        CleanUp()
                    End If
                End If

        End Select
    End Sub

    Private Sub Dither()
        ' here we say "dither" to PHD
        ' PHD don't understand "," in dither value, so we have to use "."
        ' it's necessary for non-english languages
        ' the target format of string is:
        ' {"method": "dither", "params": [10, false, {"pixels": 1.5, "time": 8, "timeout": 40}], "id": 42}
        ' more details here:
        ' https://github.com/OpenPHDGuiding/phd2/wiki/EventMonitoring
        Dim msg As String = "{""method"": ""dither"", ""params"": ["
        msg &= CStr(Int(My.Settings.Dither)) & "." & CStr(Int(10 * (My.Settings.Dither - Int(My.Settings.Dither))))
        msg &= ", false, {""pixels"": 0.5, ""time"": 15, ""timeout"": "
        msg &= CStr(My.Settings.Pause)
        msg &= "}], ""id"": 1}"
        msg &= vbCrLf
        ' Translate the passed message into ASCII and store it as a Byte array.
        Dim data As [Byte]() = System.Text.Encoding.ASCII.GetBytes(msg)
        ' stream for tcp client
        Dim stream As NetworkStream
        ' Get a client stream for reading and writing.
        stream = client.GetStream()
        ' Send the message to the connected TcpServer PHD. 
        stream.Write(data, 0, data.Length)
    End Sub

    Private Sub CleanUp()
        ' clean up things at the and of shooting session
        ' stop ticking
        Timer1.Stop()
        ' drops all variables for timer1_tick procedure to defaults
        strStatus = "Mirror"
        intCount = 0
        intFrame = 0
        ' diconnect from sdk
        EdsTerminateSDK()
        ' disconnect from PHD
        If (Not client Is Nothing) Then
            client.Close()
        End If
        ' close serial port
        If SerialPort1.IsOpen = True Then SerialPort1.Close()
        ' rename button
        btnStartStop.Text = "Start"
        ' unblock menu, exposure and repeat
        btnOptions.Enabled = True
        nbExposure.Enabled = True
        nbRepeat.Enabled = True
    End Sub

    Public Sub Statistics(ByVal TL As Integer, ByVal s As String)
        ' calculates timings and show status

        Dim TE As Integer = nbExposure.Value * nbRepeat.Value
        Dim iSpanTE As TimeSpan = TimeSpan.FromSeconds(TE) ' total exposure
        Dim iSpanTL As TimeSpan = TimeSpan.FromSeconds(TL) ' time left since start of shooting

        lbExp.Text = "Total exposure: " & iSpanTE.Hours.ToString.PadLeft(2, "0") & ":" & iSpanTE.Minutes.ToString.PadLeft(2, "0") & ":" & iSpanTE.Seconds.ToString.PadLeft(2, "0")
        lbTimeLeft.Text = "Time left: " & iSpanTL.Hours.ToString.PadLeft(2, "0") & ":" & iSpanTL.Minutes.ToString.PadLeft(2, "0") & ":" & iSpanTL.Seconds.ToString.PadLeft(2, "0")

        ' select appropriate status for lbStatus
        Select Case s
            Case "Idle"
                lbStatus.Text = "Idle"
            Case "Mirror"
                ProgressBarTotal.PerformStep()
                lbStatus.Text = "Frame " & (intFrame + 1).ToString & ": mirror settling... " & intCount.ToString & " sec"
            Case "Capture"
                ProgressBarTotal.PerformStep()
                lbStatus.Text = "Frame " & (intFrame + 1).ToString & ": capturing... " & intCount.ToString & " sec"
            Case "Pause"
                ProgressBarTotal.PerformStep()
                lbStatus.Text = "Frame " & (intFrame + 1).ToString & ": pause... " & intCount.ToString & " sec"
            Case "Abort"
                ProgressBarTotal.Value = 0
                lbStatus.Text = "Session aborted"
            Case "Complete"
                ProgressBarTotal.Value = 0
                lbStatus.Text = "Session completed"
        End Select

    End Sub

#Region "misc_small_subs"
    ' here are small gui procedures

    Private Sub nbExposure_ValueChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles nbExposure.ValueChanged
        ' update total session time if we change default value
        Statistics((nbExposure.Value + My.Settings.Mirror + My.Settings.Pause) * nbRepeat.Value - My.Settings.Pause, "Idle")
    End Sub

    Private Sub nbExposure_Leave(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles nbExposure.Leave
        ' prevent error in time calculations if user leaves field empty
        If nbExposure.Text = "" Then
            nbExposure.Text = "1"
        End If
    End Sub

    Private Sub nbRepeat_ValueChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles nbRepeat.ValueChanged
        ' update total session time if we change default value
        Statistics((nbExposure.Value + My.Settings.Mirror + My.Settings.Pause) * nbRepeat.Value - My.Settings.Pause, "Idle")
    End Sub

    Private Sub nbRepeat_Leave(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles nbRepeat.Leave
        ' prevent error in time calculations if user leaves field empty
        If nbRepeat.Text = "" Then
            nbRepeat.Text = "1"
        End If
    End Sub

    Private Sub ButtonOptions_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles btnOptions.Click
        ' shows setup menu
        Dim frm As New FormOptions
        frm.ShowDialog()
    End Sub

#End Region

End Class

Public Class CameraEventListener
    ' we need this class to bypass problems with .net Garbage Collector
    ' code below runs when camera sent event "kEdsObjectEvent_DirItemRequestTransfer"
    Public Shared Function handleObjectEvent(ByVal inEvent As Integer, ByVal inRef As IntPtr, ByVal inContext As IntPtr) As Long
        Dim rtn As Long
        ' getting file info to download
        Dim dirItemInfo As EdsDirectoryItemInfo = Nothing
        EdsGetDirectoryItemInfo(inRef, dirItemInfo)
        ' create file stream to download image
        ' image filename would be img_yyyyMMdd_HHmmss.cr2 
        ' otherwise images from new session will overwrite old ones
        ' saving path from My.Settings.Path
        Dim stream As IntPtr = Nothing
        EdsCreateFileStream(My.Settings.Path & "\img_" & Format(Now(), "yyyyMMdd_HHmmss") & ".cr2", EdsFileCreateDisposition.kEdsFileCreateDisposition_CreateAlways, EdsAccess.kEdsAccess_ReadWrite, stream)
        ' download image
        EdsDownload(inRef, dirItemInfo.size, stream)
        ' done!
        EdsDownloadComplete(inRef)
        ' free resourses
        EdsRelease(stream)
        EdsRelease(inRef)
        rtn = CLng(EDS_ERR_OK)
        Return rtn
    End Function
End Class
