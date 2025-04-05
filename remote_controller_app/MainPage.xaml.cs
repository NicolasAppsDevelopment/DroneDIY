using SkiaSharp;
using SkiaSharp.Views.Maui;
using System.Diagnostics;
using System.Net.Sockets;
using System.Net;
using System.Text;
using System.Xml.Linq;

namespace Telecommande;

public partial class MainPage : ContentPage
{
    private SKPoint RawLeftJoystickPosition;
    private bool isLeftJoystickTouched;

    private SKPoint RawRightJoystickPosition;
    private bool isRightJoystickTouched;

    private float x_LJ = 0.0f;
    private float y_LJ = 0.0f;
    private float x_RJ = 0.0f;
    private float y_RJ = 0.0f;

    CancellationTokenSource ControlSenderTskCanceller { get; } = new CancellationTokenSource();
    Socket serverSender;

    public MainPage()
    {
        InitializeComponent();

        // Subscribe to touch events
        RightJoystickCanvas.Touch += OnRightJoystickTouch;
        RightJoystickCanvas.EnableTouchEvents = true;
        LeftJoystickCanvas.Touch += OnLeftJoystickTouch;
        LeftJoystickCanvas.EnableTouchEvents = true;

        System.Globalization.CultureInfo customCulture = (System.Globalization.CultureInfo)System.Threading.Thread.CurrentThread.CurrentCulture.Clone();
        customCulture.NumberFormat.NumberDecimalSeparator = ".";
        System.Threading.Thread.CurrentThread.CurrentCulture = customCulture;

        _ = Task.Run(() => ControlSenderTask(ControlSenderTskCanceller.Token));
    }

    private async Task ControlSenderTask(CancellationToken token)
    {
        IPEndPoint ipep = new IPEndPoint(IPAddress.Parse("192.168.1.1"), 1234);
        serverSender = new Socket(AddressFamily.InterNetwork, SocketType.Dgram, ProtocolType.Udp);
        byte[] data = new byte[1024];

        do
        {
            //                                                                                 IMPORTANT pour terminer envoie sinon carte lis trop en mémoire
            string joysick_data = "JOY_DATA;" + x_LJ + ";" + y_LJ + ";" + x_RJ + ";" + y_RJ + "\0";
            data = Encoding.ASCII.GetBytes(joysick_data);
            serverSender.SendTo(data, data.Length, SocketFlags.None, ipep);
            await Task.Delay(TimeSpan.FromSeconds(0.1), token);
        } while (true);
    }

    public float Dist(float x0, float y0, float x1, float y1)
    {
        return (float)Math.Sqrt(Math.Pow(x0 - x1, 2) + Math.Pow(y0 - y1, 2));
    }

    private void OnRightJoystickPaintSurface(object sender, SKPaintSurfaceEventArgs e)
    {
        SKPoint RightJoystickPos = RawRightJoystickPosition;
        SKPoint RightJoystickCenter = new SKPoint(RightJoystickCanvas.CanvasSize.Width / 2f, RightJoystickCanvas.CanvasSize.Height / 2f);

        SKCanvas canvas = e.Surface.Canvas;
        canvas.Clear(SKColors.White);

        // Calculate joystick radius
        float joystickRadius = Math.Min(RightJoystickCanvas.CanvasSize.Width, RightJoystickCanvas.CanvasSize.Height) / 2.5f;

        if (!RightJoystickPos.IsEmpty)
        {
            // Check if the joystick position is outside the joystick bounds
            if (Dist(RightJoystickCenter.X, RightJoystickCenter.Y, RightJoystickPos.X, RightJoystickPos.Y) > joystickRadius)
            {
                // Calculate the direction from the joystick center to the raw position
                float directionX = RightJoystickPos.X - RightJoystickCenter.X;
                float directionY = RightJoystickPos.Y - RightJoystickCenter.Y;

                // Normalize the direction vector
                float magnitude = (float)Math.Sqrt(directionX * directionX + directionY * directionY);
                float normalizedDirectionX = directionX / magnitude;
                float normalizedDirectionY = directionY / magnitude;

                // Calculate the clamped position within the joystick bounds
                float clampedPositionX = RightJoystickCenter.X + normalizedDirectionX * joystickRadius;
                float clampedPositionY = RightJoystickCenter.Y + normalizedDirectionY * joystickRadius;

                // Update the joystick position to the clamped position
                RightJoystickPos = new SKPoint(clampedPositionX, clampedPositionY);
            }
        } else
        {
            RightJoystickPos = RightJoystickCenter;
        }

        x_RJ = (RightJoystickPos.X - RightJoystickCenter.X) / joystickRadius;
        y_RJ = (RightJoystickCenter.Y - RightJoystickPos.Y) / joystickRadius;

        // Draw joystick base
        SKPaint basePaint = new SKPaint
        {
            Color = SKColors.LightGray,
            Style = SKPaintStyle.Fill
        };
        canvas.DrawCircle(RightJoystickCenter.X, RightJoystickCenter.Y, joystickRadius, basePaint);

        // Draw joystick handle
        SKPaint handlePaint = new SKPaint
        {
            Color = SKColors.Red,
            Style = SKPaintStyle.Fill
        };
        canvas.DrawCircle(RightJoystickPos.X, RightJoystickPos.Y, joystickRadius / 4f, handlePaint);
    }

    private void OnLeftJoystickPaintSurface(object sender, SKPaintSurfaceEventArgs e)
    {
        SKPoint LeftJoystickPos = RawLeftJoystickPosition;
        SKPoint LeftJoystickCenter = new SKPoint(LeftJoystickCanvas.CanvasSize.Width / 2f, LeftJoystickCanvas.CanvasSize.Height / 2f);

        SKCanvas canvas = e.Surface.Canvas;
        canvas.Clear(SKColors.White);

        // Calculate joystick radius
        float joystickRadius = Math.Min(LeftJoystickCanvas.CanvasSize.Width, LeftJoystickCanvas.CanvasSize.Height) / 2.5f;

        if (!LeftJoystickPos.IsEmpty)
        {
            // Check if the joystick position is outside the joystick bounds
            if (Dist(LeftJoystickCenter.X, LeftJoystickCenter.Y, LeftJoystickPos.X, LeftJoystickPos.Y) > joystickRadius)
            {
                // Calculate the direction from the joystick center to the raw position
                float directionX = LeftJoystickPos.X - LeftJoystickCenter.X;
                float directionY = LeftJoystickPos.Y - LeftJoystickCenter.Y;

                // Normalize the direction vector
                float magnitude = (float)Math.Sqrt(directionX * directionX + directionY * directionY);
                float normalizedDirectionX = directionX / magnitude;
                float normalizedDirectionY = directionY / magnitude;

                // Calculate the clamped position within the joystick bounds
                float clampedPositionX = LeftJoystickCenter.X + normalizedDirectionX * joystickRadius;
                float clampedPositionY = LeftJoystickCenter.Y + normalizedDirectionY * joystickRadius;

                // Update the joystick position to the clamped position
                LeftJoystickPos = new SKPoint(clampedPositionX, clampedPositionY);
            }
        }
        else
        {
            LeftJoystickPos = LeftJoystickCenter;
        }

        x_LJ = (LeftJoystickPos.X - LeftJoystickCenter.X) / joystickRadius;
        y_LJ = (LeftJoystickCenter.Y - LeftJoystickPos.Y) / joystickRadius;

        // Draw joystick base
        SKPaint basePaint = new SKPaint
        {
            Color = SKColors.LightGray,
            Style = SKPaintStyle.Fill
        };
        canvas.DrawCircle(LeftJoystickCenter.X, LeftJoystickCenter.Y, joystickRadius, basePaint);

        // Draw joystick handle
        SKPaint handlePaint = new SKPaint
        {
            Color = SKColors.Blue,
            Style = SKPaintStyle.Fill
        };
        canvas.DrawCircle(LeftJoystickPos.X, LeftJoystickPos.Y, joystickRadius / 4f, handlePaint);
    }

    private void OnRightJoystickTouch(object sender, SKTouchEventArgs e)
    {
        switch (e.ActionType)
        {
            case SKTouchAction.Pressed:
                isRightJoystickTouched = true;
                RawRightJoystickPosition = e.Location;
                break;
            case SKTouchAction.Moved:
                if (isRightJoystickTouched)
                {
                    // Update joystick position based on touch position
                    RawRightJoystickPosition = e.Location;
                }
                break;
            case SKTouchAction.Released:
                isRightJoystickTouched = false;
                // Reset joystick position
                RawRightJoystickPosition = SKPoint.Empty;
                break;
        }

        // Request a redraw
        RightJoystickCanvas.InvalidateSurface();
        e.Handled = true;
    }

    private void OnLeftJoystickTouch(object sender, SKTouchEventArgs e)
    {
        switch (e.ActionType)
        {
            case SKTouchAction.Pressed:
                isLeftJoystickTouched = true;
                RawLeftJoystickPosition = e.Location;
                break;
            case SKTouchAction.Moved:
                if (isLeftJoystickTouched)
                {
                    // Update joystick position based on touch position
                    RawLeftJoystickPosition = e.Location;
                }
                break;
            case SKTouchAction.Released:
                isLeftJoystickTouched = false;
                // Reset joystick position
                RawLeftJoystickPosition = SKPoint.Empty;
                break;
        }

        // Request a redraw
        LeftJoystickCanvas.InvalidateSurface();
        e.Handled = true;
    }
}

