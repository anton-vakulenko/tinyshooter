'    Copyright (C) 2017 Anton Vakulenko 
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

Public Class FormOptions
    Private Sub ButtonSave_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles ButtonSave.Click
        ' folder for saving images
        ' note, that we don't have "/" at the end of the path
        FolderBrowserDialog1.ShowDialog()
        My.Settings.Path = FolderBrowserDialog1.SelectedPath
    End Sub

    Private Sub FormOptions_Load(ByVal sender As Object, ByVal e As System.EventArgs) Handles Me.Load
        ' reading list of all available com ports
        Dim ports As String() = IO.Ports.SerialPort.GetPortNames()
        ' add found com ports to combobox
        ComboBoxPort.Items.AddRange(ports)
        ' disable saving button if we don't want to download files to pc
        If My.Settings.Download = False Then ButtonSave.Enabled = False
    End Sub

    Private Sub FormOptions_FormClosed(ByVal sender As System.Object, ByVal e As System.Windows.Forms.FormClosedEventArgs) Handles MyBase.FormClosed
        ' update total session time because we could change default values
        Form1.TimeLeft()
    End Sub

    Private Sub CheckBoxDownload_CheckedChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles CheckBoxDownload.CheckedChanged
        ' enable of disable saving button
        If CheckBoxDownload.Checked = True Then
            ButtonSave.Enabled = True
        Else
            ButtonSave.Enabled = False
        End If
    End Sub
End Class